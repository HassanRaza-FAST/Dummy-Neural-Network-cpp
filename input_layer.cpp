#include<iostream>
#include<vector>
#include<fstream>
#include<string>
#include<pthread.h>
#include<stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include<cstring>
#include<sys/wait.h>
#include <sstream>
#include <semaphore.h>
#include <iomanip>
using namespace std;

struct ThreadParams {
    vector<float> input; //this stores the input
    vector<vector<float>> input_layer; //this stores the weight layer
    int neuron_id; //this stores the current iterative value which is used in the loop for telling which input value is being proccessed
};

void store_values(vector<float>& vec, string);
void* Neural_Network_Computation_Thread(void*);
char* trans_for_string(vector<float>);
void read_data_from_pipe(char buffer[100]);
int setting_up_semaphores(ThreadParams*);
void reading_text_file(ThreadParams *);
void calculation_for_hidden_layer();
int num_of_semaphores; //they would always be total number of layers - 1


#define PIPE_NAME "my_pipe"

// GLOBAL VARIABLES
int counter = 0;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


vector<float> ans; //this would be storing the ans, this will be converted into char* and sent to the next layer as the input


int layers; // the total number of layers
int input_neurons; // the number of input
int hidden_layer_neurons; // the number of hidden layers

sem_t **sems;
char** sem_names;

int main() {


	ThreadParams *obj= new ThreadParams;

	reading_text_file(obj);

	setting_up_semaphores(obj);


	pid_t pid;
    pid = fork(); // create a new process
    if (pid == 0) {
        // this is the child process
         char* args[] = {"./hidden", "1", "0", NULL};
   		 // Execute the new process
    	execvp(args[0], args);
    	// If execvp() returns, it means there was an error
    	perror("execvp");
    } else {
        // this is the parent process
		//int status;
        //wait(&status);
        //printf("Input layer completed. Now hidden layer will carry on its work.\n");
    }

	
	// Neural Network Processing
	pthread_t thread_id[obj->input.size()];
	for(int i = 0;i<obj->input.size();i++){
		//int *arg = new int(i);
		ThreadParams *params = new ThreadParams{obj->input, obj->input_layer,i};
		pthread_create(&thread_id[i], NULL, Neural_Network_Computation_Thread, (void*)params);
	}


	// this piece of code ensures parallel execution of threads
	pthread_mutex_lock(&mutex1);
    while (counter < obj->input.size()) {
        pthread_cond_wait(&cond, &mutex1);
    }
    pthread_mutex_unlock(&mutex1);

	cout << "\nMain Thread has returned." << endl;
	cout << "The data calculated is as following." << endl;
	for (auto i : ans) {
		cout << i << " ";
	}
	cout << endl;

	calculation_for_hidden_layer();

	sem_wait(sems[0]);
	
	char buffer[100]{};
    read_data_from_pipe(buffer);
    cout << "******" << endl;
    cout << "BACK PROPAGATING DATA RECEIVED IN INPUT LAYER = " << buffer << endl;
    cout << "******" << endl;


	

	pthread_mutex_destroy(&mutex1 ) ;

	return 0;
}

void store_values(vector<float>& vec, string line) {
	int count = 0;
	int i = 0;
	string temp_num;
	while (line[i]) {
		if (line[i] == ',') {
			vec.push_back(stof(temp_num));
			temp_num.clear();
		}
		else if (line[i] != ' ')
			temp_num += line[i];
		i++;
	}
	// for the last num
	vec.push_back(stof(temp_num));
	temp_num.clear();
}

void *Neural_Network_Computation_Thread(void *arg){

	pthread_mutex_lock ( &mutex1 ) ;
	cout << "\nLock is acquired." << endl;
	counter++;

	ThreadParams *params = (ThreadParams*)arg;
    int neuron_id = params->neuron_id;
    float neuron_val = params->input[neuron_id];
    for (int i = 0; i < ans.size(); i++) {
        ans[i] += params->input_layer[neuron_id][i] * neuron_val;
    }

	cout << "Calculation of one nueron is being done, threads are synchronised without using JOIN statements." << endl;

	cout << "Lock is unacquired." << endl;

	if (counter == params->input.size()) {
		cout << "\nAll threads have been completed in parallel." << endl;
        pthread_cond_signal(&cond);
	}

	delete params;
	pthread_mutex_unlock (&mutex1);
    pthread_exit(NULL);
}

char* trans_for_string(vector<float> vec){
	// string ans;
	// for(auto i : vec){
	// 	ans+= to_string(i);
	// 	ans+= ", ";
	// }
	// cout << ans;

	stringstream ss;
    
    // Set the precision of the float values in the vector
    ss << std::fixed << std::setprecision(2);
    
    // Convert the vector to a string
    for (const auto& val : vec) {
        ss << val << ", ";
    }
    std::string ans = ss.str();
    
    // Remove the trailing comma and space
    ans = ans.substr(0, ans.length() - 2);
	ans += "\n1";

	//cout << ans << endl;


	char* ans2 = new char[ans.length() + 1];
	strcpy(ans2, ans.c_str());
	return ans2;
}

void read_data_from_pipe(char buffer[100]){

    int fd;
   
    // Create a file descriptor for the named pipe
    fd = open(PIPE_NAME, O_RDONLY );

    // Read data from the named pipe
    read(fd, buffer, 100);

    close(fd);     
}

int setting_up_semaphores(ThreadParams* obj){
    num_of_semaphores = layers-1;


	sems = new sem_t* [num_of_semaphores];
	for (int i = 0; i < num_of_semaphores; ++i) {
    	sems[i] = new sem_t;
	}

    sem_names = new char*[num_of_semaphores];


    for(int i = 0;i<num_of_semaphores;i++){
        // Allocate memory for semaphore name
        

        // Create semaphore name
        string name = "my_semaphore" + to_string(i);
		sem_names[i] = new char[name.length()+1]{};
        for(int j = 0;j<name.length();j++){
			sem_names[i][j] = name[j]; 
		}

        // Unlink semaphore with the same name
        if (sem_unlink(sem_names[i]) == -1 && errno != ENOENT) {
            std::cerr << "Failed to unlink semaphore " << i << ": " << strerror(errno) << std::endl;
            return 1;
        }

        // Open semaphore
        sems[i] = sem_open(sem_names[i], O_CREAT | O_EXCL, 0666, 0);
        if (sems[i] == SEM_FAILED) {
            perror("sem_open");
            return 1;
        }

        // Save semaphore handle
		cout << endl << sem_names[i] << " has been made. " << endl;
    }

    return 1;
}

void reading_text_file(ThreadParams * obj){
	
	string filename = "configuration.txt";

	ifstream f_in(filename);
	string line;


	//the first line tells us about the input
	getline(f_in, line);
	size_t index = line.find("layers ");
	layers = line[index - 2]-48;
	
	index = line.find("input neurons");
	input_neurons = line[index - 2]-48;
	
	index = line.find("neurons in");
	hidden_layer_neurons = line[index - 2]-48;

	obj->input_layer.resize(input_neurons);


	// now on the normal processing
	while (getline(f_in, line)) {
		if (line.find("Input data") != string::npos) {
			getline(f_in, line);
			store_values(obj->input, line);
		}
		else if (line.find("Input layer weights") != string::npos) {
			for (int i = 0; i < input_neurons; i++) {
				getline(f_in, line);
				store_values(obj->input_layer[i], line);
			}
		}
	}

	ans.resize(obj->input_layer[0].size());

	//ans.resize(8);

	f_in.close();
	
}

void calculation_for_hidden_layer(){
	char* data_for_pipe = trans_for_string(ans);	
	size_t size = strlen(data_for_pipe);
	size++;

	// WRITING DATA INTO PIPE
	int fd;

	int result = remove(PIPE_NAME);
    if (result == 0) {
        //printf("Named pipe '%s' has been successfully deleted.\n", PIPE_NAME);
    } else {
        //printf("Error: could not delete named pipe '%s'.\n", PIPE_NAME);
    }

    /* create the named pipe */
    if (mkfifo(PIPE_NAME, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
	else{
		//cout << "Pipe has been created. " << endl;
	}

	if (access(PIPE_NAME, F_OK) == -1) {
    perror("access");
    exit(EXIT_FAILURE);
	}

    /* open the pipe for writing */
	fd = open(PIPE_NAME, O_WRONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
	else{
		cout << "\nPipe has been opened for writing. The process will now go in blocked state, unless reader process also activates." << endl;
	}

	
    /* write data to the pipe */
    if (write(fd, data_for_pipe, size) == -1) {
       perror("write");
       exit(EXIT_FAILURE);
    }
	else{
		cout << "Data writen into pipe." << endl << endl;;
	}

    close(fd);
	
	
}