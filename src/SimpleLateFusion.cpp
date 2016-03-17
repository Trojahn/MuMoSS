#include "SimpleLateFusion.hpp"

SimpleLateFusion::SimpleLateFusion(string videoPath, int vDicSize, int aDicSize, vector< pair<int,int> > keyframes, string aDescFolder, bool tempFiles) {
	this->videoPath = videoPath;
	this->vDicSize = vDicSize;
	this->aDicSize = aDicSize;
	this->keyframes = keyframes;
	this->aDescFolder = aDescFolder;
	this->tempFiles = tempFiles;
}

void SimpleLateFusion::execute() {
	string baseFile = "";
	if(this->tempFiles) {
		baseFile = Utils::calculateMD5(this->videoPath);
		cout << "The video file MD5 is '" << baseFile << "'" << endl;
	}
	
	vector<int> desiredFrames;
	for(pair<int,int> kf : keyframes) {
		desiredFrames.push_back(kf.second);
	}
	
	unsigned nThreads = thread::hardware_concurrency();
	cout << "Running some tasks with " << to_string(nThreads) << " threads" << endl;
			
	cout << "Extracting SIFT descriptors" << endl;
//	SiftExtractor sift(this->videoPath, nThreads, desiredFrames);
//	sift.extract();	
//	vector<Mat> visualDescriptors = sift.getDescriptors();
	
	
	/* This method is ULTRA SLOW. Do it in other thread! */	
	cout << "Building the visual dictionary" << endl;
//	future<Mat> fVisualDic(async(&Utils::kmeansClustering, visualDescriptors, this->vDicSize));
		
		
	/* Also slow. Do it in another thread! */
	cout << "Extracting the aural descriptors" << endl;
	vector<Mat> auralDescriptors = Utils::parseAuralDescriptors(this->aDescFolder);
	
	Mat auralDictionary;
	if(this->tempFiles) {
		if(Utils::checkFile(baseFile + "_aural_dictionary_" + to_string(this->aDicSize) + ".csv")) {
			cout << "Loading aural dictionary from file" << endl;
			auralDictionary = Utils::parseCSVDescriptor(baseFile + "_aural_dictionary_" + to_string(this->aDicSize) + ".csv");
		} else {
			cout << "Creating aural dictionary from file" << endl;
			auralDictionary = Utils::kmeansClustering(auralDescriptors, this->aDicSize);
		}
	} else {
		cout << "Creating aural dictionary from file" << endl;
		auralDictionary = Utils::kmeansClustering(auralDescriptors, this->aDicSize);
		cout << "Salving aural dictionary into a file" << endl;
		Utils::writeCSVMat(baseFile + "_aural_dictionary_" + to_string(this->aDicSize) + ".csv", auralDictionary);
	}
	
	cout << "Creating aural histograms" << endl;
	vector< vector<double> > auralHistograms = this->createHistograms(auralDescriptors, auralDictionary);
	
//	Mat visualDictionary = fVisualDic.get();
	
}

vector< vector<double> > SimpleLateFusion::createHistograms(vector<Mat> descriptors, Mat dictionary) {
	vector< vector<double> > histograms(descriptors.size());
	vector< future< vector<double> > > futures;
	
	int actual = 0;
	unsigned nThreads = thread::hardware_concurrency();
	
	for(Mat descriptor : descriptors) {
		if(futures.size() >= nThreads) {
			for(auto &f : futures) {
				histograms[actual] = f.get();
				actual++;
			}
			futures.clear();
		}
		futures.push_back(async(&Utils::extractBoFHistogram, descriptor, dictionary));
	}
	for(auto &f : futures) {
		histograms[actual] = f.get();
		actual++;
	}
	futures.clear();
	return histograms;
}


