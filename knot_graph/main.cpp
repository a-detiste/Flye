//(c) 2016 by Authors
//This file is a part of ABruijn program.
//Released under the BSD license (see LICENSE file)

#include <iostream>
#include <getopt.h>

#include "vertex_index.h"
#include "sequence_container.h"
#include "overlap.h"
#include "logger.h"
#include "config.h"
#include "assembly_graph.h"

bool parseArgs(int argc, char** argv, std::string& readsFasta, 
			   std::string& outAssembly, std::string& logFile, std::string& inAssembly,
			   int& kmerSize, int& minKmer, int& maxKmer, bool& debug,
			   size_t& numThreads, std::string& overlapsFile, int& minOverlap)
{
	auto printUsage = [argv]()
	{
		std::cerr << "Usage: " << argv[0]
				  << "\tin_assembly reads_file out_assembly \n\t\t\t\t"
				  << "[-k kmer_size] [-m min_kmer_cov] \n\t\t\t\t"
				  << "[-x max_kmer_cov] [-l log_file] [-t num_threads] [-d]\n\n"
				  << "positional arguments:\n"
				  << "\tin_assembly\tpath to input assembly\n"
				  << "\treads file\tpath to fasta with reads\n"
				  << "\tout_assembly\tpath to output assembly\n"
				  << "\noptional arguments:\n"
				  << "\t-k kmer_size\tk-mer size [default = 15] \n"
				  << "\t-m min_kmer_cov\tminimum k-mer coverage "
				  << "[default = auto] \n"
				  << "\t-x max_kmer_cov\tmaximum k-mer coverage "
				  << "[default = auto] \n"
				  << "\t-v min_overlap\tminimum overlap between reads "
				  << "[default = 5000] \n"
				  << "\t-d \t\tenable debug output "
				  << "[default = false] \n"
				  << "\t-l log_file\toutput log to file "
				  << "[default = not set] \n"
				  << "\t-o ovlp_file\tstore/load overlaps to/from file "
				  << "[default = not set] \n"
				  << "\t-t num_threads\tnumber of parallel threads "
				  << "[default = 1] \n";
	};

	kmerSize = 15;
	minKmer = -1;
	maxKmer = -1;
	numThreads = 1;
	debug = false;
	logFile = "";
	overlapsFile = "";
	minOverlap = 5000;

	const char optString[] = "k:m:x:l:t:o:v:hd";
	int opt = 0;
	while ((opt = getopt(argc, argv, optString)) != -1)
	{
		switch(opt)
		{
		case 'k':
			kmerSize = atoi(optarg);
			break;
		case 'm':
			minKmer = atoi(optarg);
			break;
		case 'x':
			maxKmer = atoi(optarg);
			break;
		case 't':
			numThreads = atoi(optarg);
			break;
		case 'v':
			minOverlap = atoi(optarg);
			break;
		case 'l':
			logFile = optarg;
			break;
		case 'o':
			overlapsFile = optarg;
			break;
		case 'd':
			debug = true;
			break;
		case 'h':
			printUsage();
			exit(0);
		}
	}
	if (argc - optind != 3)
	{
		printUsage();
		return false;
	}
	inAssembly = *(argv + optind);
	readsFasta = *(argv + optind + 1);
	outAssembly = *(argv + optind + 2);

	return true;
}

bool fileExists(const std::string& path)
{
	std::ifstream fin(path);
	return fin.good();
}

int main(int argc, char** argv)
{
	int kmerSize = 0;
	int minKmerCov = 0;
	int maxKmerCov = 0;
	int minOverlap = 0;
	bool debugging = false;
	size_t numThreads;
	std::string readsFasta;
	std::string inAssembly;
	std::string outAssembly;
	std::string logFile;
	std::string overlapsFile;

	if (!parseArgs(argc, argv, readsFasta, outAssembly, logFile, inAssembly,
				   kmerSize, minKmerCov, maxKmerCov, debugging, numThreads,
				   overlapsFile, minOverlap)) 
	{
		return 1;
	}
	Parameters::minimumOverlap = minOverlap;
	Parameters::kmerSize = kmerSize;
	Parameters::numThreads = numThreads;

	try
	{
		Logger::get().setDebugging(debugging);
		if (!logFile.empty()) Logger::get().setOutputFile(logFile);

		Logger::get().debug() << "Build date: " << __DATE__ << " " << __TIME__;
		Logger::get().debug() << "Reading FASTA";

		SequenceContainer seqAssembly; 
		seqAssembly.readFasta(inAssembly);
		SequenceContainer seqReads;
		seqReads.readFasta(readsFasta);

		VertexIndex assemblyIndex(seqAssembly);
		assemblyIndex.countKmers(1);
		assemblyIndex.buildIndex(1, 50);
		
		//getting self-overlaps for assembly
		const int MAX_JUMP = 500;
		const int NO_OVERHANGS = 0;
		OverlapDetector asmOverlapper(seqAssembly, assemblyIndex, MAX_JUMP,
							 		  Parameters::minimumOverlap, NO_OVERHANGS);
		OverlapContainer selfContainer(asmOverlapper, seqAssembly);
		if (overlapsFile.empty())
		{
			selfContainer.findAllOverlaps();
		}
		else
		{
 			if (fileExists(overlapsFile))
			{
				Logger::get().debug() << "Loading overlaps from " << overlapsFile;
				selfContainer.loadOverlaps(overlapsFile);
			}
			else
			{
				selfContainer.findAllOverlaps();
				Logger::get().debug() << "Saving overlaps to " << overlapsFile;
				selfContainer.saveOverlaps(overlapsFile);
			}
		}
		//

		AssemblyGraph ag(seqAssembly, seqReads);
		ag.construct(selfContainer);
		ag.generatePathCandidates();
		//ag.outputFasta(outAssembly);
		ag.outputDot(outAssembly + "_before");

		ag.untangle();
		ag.outputDot(outAssembly + "_after");
	}
	catch (std::runtime_error& e)
	{
		Logger::get().error() << e.what();
		return 1;
	}

	return 0;
}