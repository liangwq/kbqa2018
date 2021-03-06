#include <iostream>
#include "utils.h"
#include <pthread.h>
#include "logger.h"
#include "time.h"
#include "question_answerer.h"

question_answerer_dict* qa_dict;
pthread_mutex_t global_mutex;
int qid = 1;

void* run_qa_thread(void* dict){
	Question question;
	QuestionAnswerer thread;
	if (thread.thread_init(qa_dict) != 0){
		Logger::logging("thread init error", "ERROR");
		return NULL;
	}
	
	std::string line;
	while(1){
		pthread_mutex_lock(&global_mutex);
        	if (std::cin.eof() || !std::getline(std::cin, line)){
			pthread_mutex_unlock(&global_mutex);
			break;
		}
		std::cerr << "qid: " << qid <<std::endl;
		question.qid = qid++;
		pthread_mutex_unlock(&global_mutex);
		question.clear();
		question.original_sent = line;
		
		int ret = thread.question_answering(question, qa_dict);
		if (ret != 0){
			Logger::logging("question_answering error!", "ERROR");
			break;
		}

		std::vector<cand_answer_node*>& cands = question.cands->cand_answers;
		std::set<std::string> res;
		float max_score = 0.0;
		for (int i = 0; i < cands.size(); ++i){
			pthread_mutex_lock(&global_mutex);
			//std::cout << cands[i]->str << "\t" << cands[i]->feats["mention_net_score"] << "\t" <<  cands[i]->score << "\n";
			pthread_mutex_unlock(&global_mutex);
			if (cands[i]->score > max_score){
				max_score = cands[i]->score;
				res.clear();
				res.insert(cands[i]->str);
			}else if (cands[i]->score ==  max_score){
				res.insert(cands[i]->str);
			}
		}

		std::vector<std::string> vres;
		for (auto iter = res.begin(); iter != res.end(); ++iter){
			vres.push_back(*iter);
		}
		std::string tmp;
		Utils::join(vres, " ||| ", tmp);
		pthread_mutex_lock(&global_mutex);		
		std::cout << question.qid << "\t" << question.original_sent << "\t" << tmp << "\n";
		pthread_mutex_unlock(&global_mutex);

	}
	thread.thread_destroy();
}

int main(int argc, char *argv[]){
	pthread_mutex_init(&global_mutex, NULL);
	pthread_t pids[20];
	int thread_num = 1;
	
	if (argc < 2){
		Logger::logging("arguement error !", "ERROR");
		return -1;
	}
	if (argc >= 3){
		thread_num = Utils::str2int(argv[2]);
	}

	Logger::global_init();
	
	time_t t1, t2;
	time(&t1);
	qa_dict = QuestionAnswerer::global_init(argv[1]);
	if (qa_dict == NULL){
		Logger::logging("global init error !", "ERROR");
		return -1;
	}
	Logger::logging("global init done !", "Notice");
	time(&t2);

	std::cerr << "load time: " << difftime(t2, t1) << std::endl;
	for (int i = 0; i < thread_num; ++i){
		if (pthread_create(&pids[i], 0, run_qa_thread, NULL) != 0){
			Logger::logging("pthread init error", "ERROR");
			return -1;
		}
	}
	for (int i = 0; i < thread_num; ++i){
		pthread_join(pids[i], NULL);
	}

	QuestionAnswerer::global_destroy(qa_dict);
	return 0;
}

