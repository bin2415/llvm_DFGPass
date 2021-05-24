#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/Pass.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Use.h>
#include <llvm/Analysis/CFG.h>
#include <list>

using namespace llvm;
namespace {
	struct DFGPass : public FunctionPass {
	public:
		//ָ��Ľڵ�
		typedef std::pair<Value*, StringRef> node;
		//ָ����ָ��֮��ı�
		typedef std::pair<node, node> edge;
		//ָ��ļ���
		typedef std::list<node> node_list;
		//�ߵļ���
		typedef std::list<edge> edge_list;
		static char ID;
		//std::error_code error;
		edge_list inst_edges;  //�洢ÿ��ָ��֮����Ⱥ�ִ��˳��
		edge_list edges;    //�洢data flow�ı�
		node_list nodes;	//�洢ÿ��ָ��
		int num;
		//raw_fd_ostream *file;
		//enum sys::fs::OpenFlags F_None;
		//std::string str;
		DFGPass() : FunctionPass(ID) { num = 0; }

		//��ָ������������string��ȥ
		std::string changeIns2Str(Instruction* ins) {
			std::string temp_str;
			raw_string_ostream os(temp_str);
			ins->print(os);
			return os.str();
		}

		//����Ǳ������ñ��������֣������ָ������ָ�������
		StringRef getValueName(Value* v)
		{
			std::string temp_result = "val";
			if (!v) {
				return "undefined";
			}
			if (v->getName().empty())
			{
				temp_result += std::to_string(num);
				num++;
			}
			else {
				temp_result = v->getName().str();
			}
			StringRef result(temp_result);
			errs() << result;
			return result;
		}

		bool runOnFunction(Function &F) override {
			std::error_code error;
			enum sys::fs::OpenFlags F_None;
			StringRef fileName(F.getName().str() + ".dot");
			raw_fd_ostream file(fileName, error, F_None);
			errs() << "Hello\n";
			edges.clear();
			nodes.clear();
			inst_edges.clear();
			for (Function::iterator BB = F.begin(), BEnd = F.end(); BB != BEnd; ++BB) {
				BasicBlock *curBB = &*BB;
				for (BasicBlock::iterator II = curBB->begin(), IEnd = curBB->end(); II != IEnd; ++II) {
					Instruction* curII = &*II;
					errs() << getValueName(curII) << "\n";
					switch (curII->getOpcode())
					{
						//����load��store���ڴ���в�������Ҫ��loadָ���stroeָ������д���
						case llvm::Instruction::Load:
						{
							LoadInst* linst = dyn_cast<LoadInst>(curII);
							Value* loadValPtr = linst->getPointerOperand();
							edges.push_back(edge(node(loadValPtr, getValueName(loadValPtr)), node(curII, getValueName(curII))));
							break;
						}
						case llvm::Instruction::Store: {
							StoreInst* sinst = dyn_cast<StoreInst>(curII);
							Value* storeValPtr = sinst->getPointerOperand();
							Value* storeVal = sinst->getValueOperand();
							edges.push_back(edge(node(storeVal, getValueName(storeVal)), node(curII, getValueName(curII))));
							edges.push_back(edge(node(curII, getValueName(curII)), node(storeValPtr, getValueName(storeValPtr))));
							break;
						}
						default: {
							for (Instruction::op_iterator op = curII->op_begin(), opEnd = curII->op_end(); op != opEnd; ++op)
							{
								Instruction* tempIns;
								if (dyn_cast<Instruction>(*op))
								{
									edges.push_back(edge(node(op->get(), getValueName(op->get())), node(curII, getValueName(curII))));
								}
							}
							break;
						}

					}
					BasicBlock::iterator next = II;
					//errs() << curII << "\n";
					nodes.push_back(node(curII, getValueName(curII)));
					++next;
					if (next != IEnd) {
						inst_edges.push_back(edge(node(curII, getValueName(curII)), node(&*next, getValueName(&*next))));
					}
				}

				Instruction* terminator = curBB->getTerminator();
				for (BasicBlock* sucBB : successors(curBB)) {
					Instruction* first = &*(sucBB->begin());
					inst_edges.push_back(edge(node(terminator, getValueName(terminator)), node(first, getValueName(first))));
				}
			}
			//errs() << "Write\n";
			file << "digraph \"DFG for'" + F.getName() + "\' function\" {\n";
			//errs() << "Write DFG\n";
			//��node�ڵ�dump
			for (node_list::iterator node = nodes.begin(), node_end = nodes.end(); node != node_end; ++node) {
				//errs() << "Node First:" << node->first << "\n";
				//errs() << "Node Second:" << node-> second << "\n";
				if(dyn_cast<Instruction>(node->first))
					file << "\tNode" << node->first << "[shape=record, label=\"" << *(node->first) << "\"];\n";
				else
					file << "\tNode" << node->first << "[shape=record, label=\"" << node->second << "\"];\n";
			}
			//errs() << "Write Done\n";
			//��inst_edges��dump
			for (edge_list::iterator edge = inst_edges.begin(), edge_end = inst_edges.end(); edge != edge_end; ++edge) {
				file << "\tNode" << edge->first.first << " -> Node" << edge->second.first << "\n";
			}
			//��data flow�ı�dump
			file << "edge [color=red]" << "\n";
			for (edge_list::iterator edge = edges.begin(), edge_end = edges.end(); edge != edge_end; ++edge) {
				file << "\tNode" << edge->first.first << " -> Node" << edge->second.first << "\n";
			}
			errs() << "Write Done\n";
			file << "}\n";
			file.close();
			return false;
		}

	};
}

char DFGPass::ID = 0;
static RegisterPass<DFGPass> X("DFGPass", "DFG Pass Analyse",
	false, false
);
