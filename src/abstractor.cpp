/**
 * @file abstractor.cpp
 * @author Elif Kızılkaya
 *
 *  @brief This code is designed to create a multithreaded scientific search engine
 *  that queries the paper abstracts and summarizes the most relevant ones. It
 *	take two 2 arguments as input_file_name.txt and output_file_name.txt. After
 *	taking the required information such as number of thread, number of abstracts,
 *	query to be compared, and number of results returned, it creates the threads.
 *	These threads runs the abstractor function and calculates the similarity between
 *	the abstract text and query. Also it summarizes the most relevant sentence. These 
 *	results are stored in the parameters of the abstractor function and used in the main
 *	function to write the results in desired way to the output file given in the
 *	command arguments. To meet the synchronization issues, mutex is used.
 *
 * ATTENTION PLEASE:
 * input_file_name.txt and output_file_name.txt should be only names, not whole paths
 *
 * How to compile and run:
    make
    ./abstractor.out input_file_name.txt output_file_name.txt


 * ANOTHER NOTE:
 	In my code, if thread numbers > number of abstracts texts, then every thread works once
 	and leave the job to others.

 *	I make use of the following websites:
 *	https://stackoverflow.com/questions/8223742/how-to-pass-multiple-parameters-to-a-thread-in-c
 *	https://linux.die.net/man/3/pthread_getname_np
 *	https://stackoverflow.com/questions/2369738/how-to-set-the-name-of-a-thread-in-linux-pthreads
 *
 */

#include <pthread.h>
#include <cctype>
#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <fstream>
#include <vector>
#include <string.h>
#include <iomanip>
#include <iostream>
#include <unistd.h>
#include <bits/stdc++.h>

using namespace std;

#define namelen 16
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER; //to avoid the conflicts between threads
ofstream outfile; //to write the output file
ifstream infile; //to read from input file
int read_index; //to read the abstracts from thread function from the resumin index
int total_line; //total lines in the input file
int num_waiting; //waiting threads numbers
int num_thread; //total number of thread
int num_text; //number of abstracts texts
vector<vector<string>> summation; //Summaries are put in here
vector<double> similarity; //Similarity calculations are put in here
vector<string> file_names; //The name of the abstract file corresponding to similartiy and summation in that index


void* abstractor( void *arg);
vector<int> maxIndexes(vector<double> vec, int number);


/*
	The struct for the parameters send to abstractor()
	function while creating thread
*/
typedef struct parameters { 
	vector<string> name_text;
	string query;
 } parameters;

/*
	Reads the input file and creates the threads accordingly
	After getting the results from threads which run abstractor()
	function, writes to output file given in the command line argument.
*/

int main(int argc, char const *argv[])
{
	string infile_name = argv[1];
	string outfile_name = argv[2];
	
	infile.open(infile_name); //openning the input file
	outfile.open(outfile_name); //openning the output file
	
	int num_output = 0;
	string query;
	
	/*
		Reading the information provided by input file
	*/
	infile >> num_thread;
	infile >> num_text;
	infile >> num_output;
	infile.ignore(); //Since the previous statement left at newline, I used ignore() function
	getline(infile, query);

	read_index = 3; //Abstractor text names start from line 3
	total_line = 2 + num_text; //total number of lines
	num_waiting = num_thread; //at first, they are all empty
	
	pthread_t threads[num_thread];
	pthread_attr_t attr; // set of thread attributes 
	
	// set the default attributes of the threads 
    pthread_attr_init(&attr);
    
    parameters params[num_thread]; //parameter for each thread
  
  	/*
  		To name the threads, a char pointer is used.
  		It points a char value and value increases at
  		each iteration.
  	*/
    char b = 'A';
    char *a = &b;
    

	for (int i = 0; i < num_thread; ++i)
	{

    	params[i].query = query;
		pthread_create(&threads[i], &attr, abstractor, &params[i]);
		pthread_setname_np(threads[i], a);
		b++;
		
	}

	/*
		Threads join
	*/
	for (int i = 0; i < num_thread; ++i)
	{
		pthread_join(threads[i], NULL);
	}

	/*
		Writing the results to output file in the desired
		way.
	*/
	outfile << "###" << endl;

	vector<int> max_indexes =  maxIndexes(similarity,num_output);

  	for (int i = 0; i < num_output; ++i)
  	{
  		int maximum = max_indexes[i];
  		outfile << "Result " << i + 1 <<":\nFile: " << file_names[maximum] << "\nScore: ";
  		outfile << std::fixed << std::setprecision(4) << similarity[maximum] << "\n";
  		outfile << "Summary:";

  		for (int l = 0; l < summation[maximum].size(); ++l)
		{
			string res = summation[maximum][l];
			res.erase(std::remove(res.begin(), res.end(), '\n'), res.end()); //Clearing the summary from newlines

			if (res[0] != ' ') //In the beginning of the string, there can be empty space
			{
				outfile << " " << res << ".";
			} else {
				outfile << res << ".";
			}
		}
		outfile << " " << endl;
		outfile << "###" << endl;
  	}

	infile.close();
	outfile.close();
	
	return 0;
}

/*
	Each thread runs this function. It reads the abstract text
	name from the input file and increases the read_index so 
	that the other threads can continue with other abstract texts.
	Afterwards, it opens the abstract text and reads the sentences
	and calculates the similarity with the given query in the parameters
	using Jaccard Similarity Metric and pushes them to similarity vector. 
	Also, it finds the sentences that includes common words with query 
	and adds them to ext_summation vector. Mutex locks are used while
	dealing with global variables such as similarity, ext_summation, infile
	and outfile variables.
*/

void* abstractor( void *arg){

	parameters* ptr =(parameters *)arg;
	string query = ptr->query; //Query provided by input file
	string abstract_text;

/* 
	There is a infite loop because if there is a text to read 
	and there is no other thread, it should continue to work.
*/
	while(true){
		pthread_mutex_lock(&mutex1);
		
		if(read_index > total_line){ //If there is not any abstract text left, break
			pthread_mutex_unlock(&mutex1);
			break;
		}

		read_index++; //One abstract text will be read
		num_waiting--; //One thread started to work
		char buf[16];
		pthread_getname_np(pthread_self(),buf, namelen); //Getting the name of the thread
		

		getline(infile, abstract_text);
		char name = toupper(buf[0]);	
		outfile << "Thread " << name << " is calculating " << abstract_text << endl;
	
		pthread_mutex_unlock(&mutex1);

	/*
		Reading the abstract_text
	*/
		ifstream read_file;
		string path = "../abstracts/" + abstract_text;
		read_file.open(path);
		
		ptr->name_text.push_back(abstract_text);

		stringstream stream;
	    stream << read_file.rdbuf(); //read the file
	    string text = stream.str(); //str holds the content of the abstract file

	    vector<string> query_arr;

		int num_query = 0; //after while loop, it will indicate how many words there are in query
		stringstream ss(query);
		string qu_token;
	
		while(ss >> qu_token){ //Separating the query into tokens
		
			query_arr.push_back(qu_token);
			num_query++;
		}

		vector<string> separated_lines; //holds the lines separated with '.'
		vector<string> ext_summation;
		int tot_lines = 0; //index, also indicates the number of lines in text after while statement
		
		int sum_counter = 0;
		stringstream ss2(text);
		string token;
		while (getline(ss2, token, '.')) { 
			
			separated_lines.push_back(token);
			tot_lines++;
		
			bool check = false; //to check that line is written only one time to the summation
			stringstream ss3(token); //To tokenize the each sentence in abstract text
			string outtoken;
					
			while(ss3 >> outtoken){
				
				//Iterating for all words in query if there is a match
				for (int r = 0; r < num_query; r++){ //num_query is number of words in query
					
					if (outtoken == query_arr.at(r)){
						if(!check){ //If the sentence is not selected before, continue
							ext_summation.push_back(separated_lines.at(tot_lines-1));
						    sum_counter++;
						  	check = true;
						}
						    	
						break; //break after writing ro sentence, because only one common words is enough
					}
				}    	
			}		
		}
		
		/*
			Finding the unique words 
		*/
		int position = 0;
		int index_str;
		string unique = " "; //holds unique words
		string un_lines = text; //Tokenize the whole abstract text
		stringstream ss4(un_lines);
		string un_token;
		int num_unique = 0; //number of unique words
		while(ss4 >> un_token){
			 if (!(strstr(unique.c_str(),(" "+un_token + " ").c_str())) ) //If unique variable does not contain the token, then add
			 {
			 	unique += un_token + " ";
			    num_unique++;
			 }                               
		}

		int intersection = 0;
		stringstream ss5(unique);	
		string intoken;
		/* 
			Check if the unique words in the abstract text
			 has intersection with the query
		*/
		while(ss5 >> intoken){
			for (int h = 0; h < num_query; ++h){	
				if (intoken == query_arr.at(h)){
			    	intersection++;
				}
			}
		}

		/*
			union =
			(total number of words in query + unique words in abstract text - intersection of them)
		*/
		int uni_on = num_query + num_unique; 
		uni_on -= intersection;
		/*
			Jaccard Similarity Metric formula
		*/
		double simil = (double) intersection / uni_on;

		/*
			Add the results to global variable.
			Mutex is required
		*/
		pthread_mutex_lock(&mutex1);
		similarity.push_back(simil); //corresponding similarity
		summation.push_back(ext_summation); //corresponding summary
		file_names.push_back(abstract_text); //corresponding abstract_text name
		pthread_mutex_unlock(&mutex1);
			
		pthread_mutex_lock(&mutex1);

		/*
			If there are more threads then number of abstracts,
			then leave the work to the others so that job can
			be distributed evenly.
		*/
		if(num_thread > num_text && num_waiting > 0){
			pthread_mutex_unlock(&mutex1);
			break;
		}
		pthread_mutex_unlock(&mutex1);
		/*
			To distribute the job evenly,
			sleep a little bit to start work again
		*/
		sleep(0.002);
		
		read_file.close();
		}
		
		pthread_exit(NULL);
	}


/*
	Return a vector of the indexes that points to maximum elements of
	the double vector. Size of the returned vector is specified with 
	the parameter, number.
*/
vector<int> maxIndexes(vector<double> vec, int number){

	double max = 0.0;
	vector<int> index;
	vector<bool> isMax(vec.size(),false); //none of them is max in the begining
	int max_index = 0;

	for (int i = 0; i < number; ++i)
	{
		max = 0.0;
		for (int k = 0; k < vec.size(); ++k)
		{
			if(vec.at(k) >= max && isMax[k] == false){ //Find the maximum number and the index of it
				max = vec.at(k);
				max_index = k;
		
			}
		}
		isMax[max_index] = true;
		index.push_back(max_index);
		
		}

	return index;

}