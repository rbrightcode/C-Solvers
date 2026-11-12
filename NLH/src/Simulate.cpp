// MEET THE FIRST HOLD EM BOT I AM JUST TESTING RN SO ALOT OF THINGS DONT EXIST... Like abstraction,
// Lets start by benchmarking speed. and prayinng it works...

#include "../include/NLH.h"
#include <chrono>
#include "../include/Exploitability.h"
#include "../include/Buckets.h"

int main() {
	std::cout << "How many iterations do you want to train the bot for?\n";
	int iter;
	std::cin >> iter;
	auto time0 = std::chrono::high_resolution_clock::now();

	NLH trainer;
	trainer.load_model("Data/Model.bin");

	trainer.train(iter);

	trainer.save_model("Data/Model.bin");

	auto time1 = std::chrono::high_resolution_clock::now();
	std::cout << "Training Done. Time elapsed: " << ((time1 - time0) / 1e9).count() << " Seconds\n";

	std::cout << trainer.nodes.size() << std::endl;

	Blueprint bp(trainer);
	std::vector<Node>().swap(trainer.nodes);
	std::unordered_map<uint64_t, uint32_t>().swap(trainer.history_lookup);

	auto time2 = std::chrono::high_resolution_clock::now();
	std::cout << "Blueprint Done. Time elapsed: " << ((time2 - time1) / 1e9).count() << " Seconds\n";

	Exploitability exp(trainer);

	float ep = exp.getExploitability(bp);
	std::cout << "Exploitability: " << ep << std::endl;	

	auto time3 = std::chrono::high_resolution_clock::now();
	std::cout << "Exploitability Done. Time elapsed: " << ((time3 - time2) / 1e9).count() << " Seconds\n";
	
	return 0;
};

// FIRST TRIAL RUN 1 iteration. 20 minutes, Chance sampling, no outcome sampling.
// Add outcome sampling
// Trial run 2: outcome sampling. 100 iterations, 9 seconds, 200 17?, 1000 30, 2000 110
// Improve precomputing for showdown.
// Trial run 3: Turn + River buckets, 10000... 1000 17, 2000 44, 5000 110, 10000

// Trial run 4: Switched to single combo MCCFR with external sampling, This means we will need a 1000 x more iteration.
// In terms of touched nodes this is currently slower than range cfr. however, it has more accurate sampling and allows for pruning which has a major upside.
// 100,000 in 84 seconds = 1200 / second... Precompute perm_id 100,000 in 47 = 2000 / second... improve state 100,000 in 32 = 3000 / second
// Precompute perm_id


// ES + LMCCFR 1000 per second, node lookup is bottle neck 20000 samples exploitability of 37.

// 1 sample; EXP: 61
// 1000000 samples; EXP: 52, 47
// 10000000 samples; EXP: 36
// 100000000 samples; EXP: 33


//Cumulative Runs
// 1 mil: exp: 50
// 5 mil: exp: 35
// 10 mil: exp: 31
// 20 mil: exp: 31
// 50 : 36 | 52 : 33
// 150: 29, 33