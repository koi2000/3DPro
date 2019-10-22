/*****************************************************************************
* Copyright (C) 2011 Adrien Maglo and Clément Courbet
*
* This file is part of PPMC.
*
* PPMC is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* PPMC is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with PPMC.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/
#include "ppmc.h"

#include <unistd.h>
#include <sys/time.h>
#include <queue>

using namespace CGAL;
using namespace std;

/*
 * ppmc takes objects from standard in
 * compress them to a binary file, and output the offset
 * and length information together with the MBB information
 * of each object to the standard out.
 * note that, the mbb information for all the objects processed
 * by this instance will also be attached to the end of the binary file
 *
 * */

struct timeval get_cur_time(){
	struct timeval t1;
	gettimeofday(&t1, NULL);
	return t1;
}
double get_time_elapsed(struct timeval t1){
	struct timeval t2;
    double elapsedTime;
	gettimeofday(&t2, NULL);
	// compute and print the elapsed time in millisec
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
	return elapsedTime;
}

int i_mode = COMPRESSION_MODE_ID; // compression mode
unsigned i_quantBit = 12;
unsigned i_decompPercentage = 100;
bool optimization = false;
bool b_useAdaptiveQuantization = optimization;
bool b_useLiftingScheme = optimization;
bool b_useCurvaturePrediction = optimization;
bool b_useConnectivityPredictionFaces = optimization;
bool b_useConnectivityPredictionEdges = optimization;
bool b_allowConcaveFaces = true;
bool b_useTriangleMeshConnectivityPredictionFaces = true;
std::ofstream *myFile;
long total_size = 0;
int num_objects = 0;
struct timeval start_time;

pthread_mutex_t line_lock;
pthread_mutex_t output_lock;
bool stop = false;

#define QUEUE_SIZE 100
int NUM_THREADS = 10;
int global_index = 0;
#define MAX_THREAD_NUM 100
string processing_line[MAX_THREAD_NUM];
bool is_working[MAX_THREAD_NUM];

void *compress(void *args){
	int id = *(int *)args;
	pthread_mutex_lock(&output_lock);
	cout<<"thread "<<id<<" is started"<<endl;
	pthread_mutex_unlock(&output_lock);

	// output binary file for the compressed data

	char *buffer = new char[BUFFER_SIZE];
	char *currentPos = buffer;
	int offset = 0;

	 // Temporary line
	while (!stop) {
		if(!is_working[id]){
			usleep(10);
			continue;
		}
		pthread_mutex_lock(&line_lock);
		int cur_index = global_index++;
		pthread_mutex_unlock(&line_lock);
		// the input format is "OFF|numbers|numbers|...."
		/* Parsing polyhedron input */
		try {
			// convert it back to a normal OFF format
			boost::replace_all(processing_line[id], "|", "\n");
			// Init the random number generator.
			srand(PPMC_RANDOM_CONSTANT);
			MyMesh *currentMesh = new MyMesh(i_decompPercentage,
						 i_mode, i_quantBit,
						 b_allowConcaveFaces,
						 processing_line[id].c_str(), processing_line[id].size());
			currentMesh->completeOperation();

			// if the buffer is full, write the compressed data into binary file
			if(offset+currentMesh->dataOffset>=BUFFER_SIZE){
				pthread_mutex_lock(&output_lock);
				myFile->write(buffer, offset);
				total_size += offset;
				pthread_mutex_unlock(&output_lock);
				currentPos = buffer;
				offset = 0;
			}

			// copy to buffer
			memcpy(currentPos, currentMesh->p_data, currentMesh->dataOffset);
			currentPos = currentPos + currentMesh->dataOffset;
			offset += currentMesh->dataOffset;

			delete currentMesh;
			if(cur_index%100==0){
				cout<<"thread "<<id<<" processed "<<cur_index<<endl;
			}
		} catch (const std::exception &exc) {
			std::cerr << "******Geometry Parsing Error******" << std::endl;
			std::cerr << exc.what()<<std::endl;
			exit(-1);
		}
		processing_line[id].clear();
		is_working[id] = false;
	} // end of while
	// writing last chunk
	if (offset > 0){
		pthread_mutex_lock(&output_lock);
		myFile->write(buffer, offset);
		total_size += offset;
		pthread_mutex_unlock(&output_lock);
	}
	delete[] buffer;
	pthread_exit(NULL);
	return NULL;
}


int main(int argc, char** argv) {
	if(argc>1){
		NUM_THREADS = atoi(argv[1]);
	}

	assert(NUM_THREADS>0 && NUM_THREADS<MAX_THREAD_NUM);
	start_time = get_cur_time();
	pthread_t threads[NUM_THREADS];
	int id[NUM_THREADS];
	for(int i=0;i<NUM_THREADS;i++){
		id[i] = i;
		processing_line[i].clear();
		is_working[i] = false;
	}
	// write the compressed data
	myFile = new std::ofstream("output", std::ios::out | std::ios::binary);

	for(int i=0;i<NUM_THREADS;i++){
		int rc = pthread_create(&threads[i], NULL, compress, (void *)&id[i]);
		if (rc) {
			cout << "Error:unable to create thread," << rc << endl;
			exit(-1);
		}
	}
	std::string input_line;
	std::vector<string> lines;
	while (std::cin && getline(std::cin, input_line) && !std::cin.eof()) {
		while(true){
			bool assigned = false;
			for(int i=0;i<NUM_THREADS;i++){
				if(is_working[i]==false){
					processing_line[i] = input_line;
					is_working[i] = true;
					assigned = true;
					break;
				}
			}
			if(assigned){
				break;
			}
			usleep(10);
		}
	}
	stop = true;

	for(int i = 0; i < NUM_THREADS; i++ ){
		void *status;
		int rc = pthread_join(threads[i], &status);
		if (rc) {
			cout << "Error:unable to join," << rc << endl;
			exit(-1);
		}
		cerr << "Main: completed thread id :" << i ;
		cerr << "  exiting with status :" << status << endl;
	}

	myFile->flush();
	myFile->close();

    std::cerr <<"processed "<<num_objects<<" objects in "<<get_time_elapsed(start_time)/1000<<" seconds"<<endl;
	std::cerr <<"total size of compressed data is "<< total_size << std::endl; // the total size

	cerr << "Main: program exiting." << endl;
	pthread_exit(NULL);

	return true;
}
