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
#include<sstream>
#include<iomanip>
#include <semaphore.h>
using namespace std;

string filename = "configuration.txt";

int output_layer_neurons;

struct ThreadParams {
    vector<float> input;
    vector<float> input_layer;
    int neuron_id;
};

#define PIPE_NAME "my_pipe"

// GLOBAL VARIABLES
int counter = 0;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
float ans;

int num_of_semaphores;

sem_t *sem;
char* sem_name; // the output layer only communicates with the last semaphore

vector<float> output_layer;

vector<float> do_preprocessing();
void store_values(vector<float>& vec, string line);
void *Neural_Network_Computation_Thread(void *arg);
char* trans_for_string(vector<float>);

int main(int argc, char* argv[]){


    vector<float> pipe_data = do_preprocessing();

    string name = "my_semaphore" + to_string(num_of_semaphores-1); //-1 because 0 based indexing
	sem_name = new char[name.length() + 1]{};
	for(int i = 0;i<name.length();i++){
        sem_name[i] = name[i];
    }

	sem = sem_open(sem_name, 0);
    if(sem==SEM_FAILED){
        perror("sem_open");
        return 1;
    }
    cout << sem_name << " opened." << endl;

	// sem5 = sem_open(sem_name5, 0);
    // if (sem5 == SEM_FAILED) {
    //     perror("sem_open");
    //     return 1;
    // }
    

    cout << "\nPreprocessing has been done. " << endl;

    ThreadParams *obj= new ThreadParams;
    obj->input = pipe_data;
    obj->input_layer = output_layer;
    
    cout << "Output Layer Input: " << endl;
    for(auto i : obj->input){
        cout << i << " ";
    }
    cout << endl;

    cout << "Output layer weights: " << endl;
    for(auto it:obj->input_layer){
        cout << it << " ";
    }
    cout << endl;

    pthread_t thread_id[output_layer_neurons];
	for(int i = 0;i<output_layer_neurons;i++){
		//int *arg = new int(i);
		ThreadParams *params = new ThreadParams{obj->input, obj->input_layer,i};
		pthread_create(&thread_id[i], NULL, Neural_Network_Computation_Thread, (void*)params);
	}

    // this piece of code ensures parallel execution of threads
	pthread_mutex_lock(&mutex1);
    while (counter < output_layer_neurons) {
        pthread_cond_wait(&cond, &mutex1);
    }
    pthread_mutex_unlock(&mutex1);

    cout << "*****" << endl;
    cout << "FINAL ANS = " << ans << endl;
    cout << "*****" << endl;

    cout << "OUTPUT process done. Now backpropagating output to last hidden layer." << endl;
    //write data into pipe so last layer can read it

    string ans_str = to_string(ans);
    char* data_for_pipe = new char[ans_str.length() + 1];
    strcpy(data_for_pipe, ans_str.c_str());
 	
	size_t size = strlen(data_for_pipe);
	size++;

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

     sem_post(sem);  
    

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
        cout << "Data writen into pipe." << endl << endl;
    }


    /* close the pipe */
    close(fd);

    sem_close(sem);
    sem_unlink(sem_name);
    
    pthread_mutex_destroy(&mutex1 ) ;

    return 0;
}



vector<float> do_preprocessing(){
    int fd;
    char buffer[100]{};

    // Create a file descriptor for the named pipe
    fd = open(PIPE_NAME, O_RDONLY );

    // Read data from the named pipe
    read(fd, buffer, 100);

    // Process the data
    cout << "Received data: " << buffer << endl;

    // Close the file descriptor
    close(fd);

    string vec_str(buffer);
    char delimiter = '\n';
    size_t pos = vec_str.find(delimiter);
    string new_vec_str = vec_str.substr(0, pos + 1);

    //std::string vec_str = "-0.07, 0.08, 0.15, -0.07, 0.08, 0.15, -0.07, 0.08\n";

    // string substr = vec_str.substr(vec_str.find('\n') + 1); // extract substring after \n
    // stringstream ss1(substr); // create stringstream object from substring

    // double num;
    // ss1 >> hidden_layer_id; // read first number in stringstream

    // Create a stringstream object and initialize it with the string
    std::stringstream ss(new_vec_str);

    // Create a new vector to store the float values
    std::vector<float> new_vec;

    // Extract the float values from the stringstream and add them to the new vector
    float val;
    while (ss >> val) {
        new_vec.push_back(val);
        // Ignore the comma and space characters
        ss.ignore();
    }

    // // Print the new vector to verify the conversion
    // for (const auto& v : new_vec) {
    //     std::cout << v << " ";
    // }
    // std::cout << std::endl;

    //now read configuration.txt
    string filename = "configuration.txt";

	ifstream f_in(filename);
	string line;

    output_layer_neurons = new_vec.size(); // as we know that row is the number of neurons in previous layer

    f_in.close();
    cout << "Output layer neurons count = " << output_layer_neurons << endl;

    ifstream f_in2(filename);

    // read file again

    while (getline(f_in2, line)) {
        if (line.find("layers ")!= string::npos){
            size_t index = line.find("layers ");
	        num_of_semaphores = line[index - 2]-48;
            num_of_semaphores--; //since always total layers - 1
        }
		else if (line.find("Output layer weights") != string::npos) {
			for (int i = 0; i < output_layer_neurons; i++) {
				getline(f_in2, line);
				store_values(output_layer, line);
			}
		}
	}

    return new_vec;
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
    ans += params->input_layer[neuron_id] * neuron_val;

    cout << "Calculation of one nueron is being done, threads are synchronised without using JOIN statements." << endl;


	cout << "Lock is unacquired." << endl;

	if (counter == output_layer_neurons) {
		cout << "\nAll threads have been completed in parallel." << endl << endl;
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
	ans += "\n";
    //ans+=to_string(hidden_layer_id+1);

	//cout << ans << endl;


	char* ans2 = new char[ans.length() + 1];
	strcpy(ans2, ans.c_str());
	return ans2;
}

