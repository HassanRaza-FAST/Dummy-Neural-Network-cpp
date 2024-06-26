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
#include <chrono>
#include <semaphore.h>
using namespace std;



int hidden_layer_neurons;
string filename = "configuration.txt";
int hidden_layer_id;

int processss = 1;
int proc_counter;


struct ThreadParams {
    vector<float> input;
    vector<vector<float>> input_layer;
    int neuron_id;
};

#define PIPE_NAME "my_pipe"

// GLOBAL VARIABLES
int counter = 0;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
vector<float> ans;

vector<vector<float>> hidden_layer1;

vector<float> do_preprocessing();
void store_values(vector<float>& vec, string line);
void *Neural_Network_Computation_Thread(void *arg);
char* trans_for_string(vector<float>);
void read_data_from_pipe(char[100]);
void write_data_into_pipe(char[100]);
void setting_up_pipe();
void write_data_into_pipe_front(char*, size_t);
int setting_up_semaphores();

int num_of_semaphores; //they would always be total number of layers - 1

sem_t **sems;
char** sem_names;

int main(int argc, char* argv[]){

    //  string t(argv[1]);
    //  processss =  stoi(t);

    vector<float> pipe_data = do_preprocessing();

    cout << "\nPreprocessing has been done. " << endl;

    setting_up_semaphores();

    ThreadParams *obj= new ThreadParams;
    obj->input = pipe_data;
    obj->input_layer = hidden_layer1;
    ans.resize(hidden_layer_neurons);

    cout << "\nHidden layer " << processss << " Input:" << endl; 
    for(auto i : obj->input){
        cout << i << " ";
    }
    cout << endl;

    cout << "\nHidden layer " << processss << " Weights:" << endl; 

    for(auto it:obj->input_layer){
        for(auto i : it){
            cout << i << " ";
        }
        cout << endl;
    }
    cout << endl;

    pthread_t thread_id[hidden_layer_neurons];
	for(int i = 0;i<hidden_layer_neurons;i++){
		//int *arg = new int(i);
		ThreadParams *params = new ThreadParams{obj->input, obj->input_layer,i};
		pthread_create(&thread_id[i], NULL, Neural_Network_Computation_Thread, (void*)params);
	}

    // this piece of code ensures parallel execution of threads
	pthread_mutex_lock(&mutex1);
    while (counter < hidden_layer_neurons) {
        pthread_cond_wait(&cond, &mutex1);
    }
    pthread_mutex_unlock(&mutex1);

    cout << "\nMain Thread has returned." << endl;
	cout << "The data calculated is as following." << endl;

    for(auto i:ans){
        cout << i << " ";
    }

    cout << endl;

    
	char* data_for_pipe = trans_for_string(ans);	
	size_t size = strlen(data_for_pipe);
	size++;

    // string t(argv[1]);
    // processss =  stoi(t);
    // string t1(argv[2]);
    // proc_counter = stoi(t1);

    pid_t pid;
    pid = fork(); // create a new process
    proc_counter++;
    cout << getpid() << " : " << proc_counter << " :p " << processss << endl;
    if(processss<5){
        if (pid == 0) {
            // this is the child process
            processss++;
            string temp = to_string(processss);
            char *cstr = new char[temp.length() + 1];
            strcpy(cstr, temp.c_str());
            temp = to_string(proc_counter);
            char *cstr1 = new char[temp.length() + 1];
            strcpy(cstr1, temp.c_str());
            char* args[] = {"./hidden", cstr, cstr1, NULL};
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
    }
    else if(processss==5){
        if(pid==0){
            // we need to go to output now
            char* args[] = {"./output", "1", NULL};
            // Execute the new process
            execvp(args[0], args);
            // If execvp() returns, it means there was an error
            perror("execvp");
        }
    }

    if(processss<=5){
        setting_up_pipe();
        write_data_into_pipe_front(data_for_pipe, size);
    }


    for(int i = 0 ; i < num_of_semaphores; i++){
        if(processss == i){
            sem_wait(sems[i]);
             char buffer[100]{};
            read_data_from_pipe(buffer);
            cout << "******" << endl;
            cout << "HIDDEN LAYER NUMBER " << i << " - BACK PROPAGATING DATA RECEIVED:  " << buffer << endl;
            cout << "******" << endl;
        
            setting_up_pipe();
    
            sem_post(sems[i-1]);  
    
            write_data_into_pipe(buffer);

         sem_close(sems[i-1]); 
         sem_unlink(sem_names[i-1]);
        }
    }

    // if(processss==5){
    //     sem_wait(sem5);
    //     char buffer[100]{};
    //     read_data_from_pipe(buffer);
    //     cout << "BACK PROPAGATING DATA RECEIVED IN 5TH LAYER " << buffer << endl;
        
    //    setting_up_pipe();
    
    //     sem_post(sem4);  
    
    //     write_data_into_pipe(buffer);

    //     sem_close(sem4); 
    //     sem_unlink(sem_name4);
        
    // }
    // else if (processss==4){
    //     sem_wait(sem4);
    //     char buffer[100]{};
    //     read_data_from_pipe(buffer);
    //     cout << "BACK PROPAGATING DATA RECEIVED IN 4TH LAYER " << buffer << endl;

    //     setting_up_pipe();
    //     sem_post(sem3);  
    //     write_data_into_pipe(buffer);
    //     sem_close(sem3); 
    //     sem_unlink(sem_name3);
    // }
    // else if (processss==3){
    //     sem_wait(sem3);
    //     char buffer[100]{};
    //     read_data_from_pipe(buffer);
    //     cout << "BACK PROPAGATING DATA RECEIVED IN 3rd LAYER " << buffer << endl;

    //     setting_up_pipe();
    //     sem_post(sem2);  
    //     write_data_into_pipe(buffer);
    //     sem_close(sem2); 
    //     sem_unlink(sem_name2);
    // }
    // else if (processss==2){
    //     sem_wait(sem2);
    //     char buffer[100]{};
    //     read_data_from_pipe(buffer);
    //     cout << "BACK PROPAGATING DATA RECEIVED IN 2nd LAYER " << buffer << endl;

    //     setting_up_pipe();
    //     sem_post(sem1);  
    //     write_data_into_pipe(buffer);
    //     sem_close(sem1); 
    //     sem_unlink(sem_name1);
    // }
    // else if(processss==1){
    //     sem_wait(sem1);
    //     char buffer[100]{};
    //     read_data_from_pipe(buffer);
    //     cout << "BACK PROPAGATING DATA RECEIVED IN 1st LAYER " << buffer << endl;

    //     setting_up_pipe();
    //     sem_post(sem);  
    //     write_data_into_pipe(buffer);
    //     sem_close(sem); 
    //     sem_unlink(sem_name);
    // }
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
    cout << "Received data from pipe: " << buffer << endl;
    cout << endl;

    // Close the file descriptor
    close(fd);

    string vec_str(buffer);
    char delimiter = '\n';
    size_t pos = vec_str.find(delimiter);
    string new_vec_str = vec_str.substr(0, pos + 1);

    //std::string vec_str = "-0.07, 0.08, 0.15, -0.07, 0.08, 0.15, -0.07, 0.08\n";

    string substr = vec_str.substr(vec_str.find('\n') + 1); // extract substring after \n
    stringstream ss1(substr); // create stringstream object from substring

    ss1 >> hidden_layer_id; // read first number in stringstream
    processss = hidden_layer_id;

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

    // Print the new vector to verify the conversion
    for (const auto& v : new_vec) {
        std::cout << v << " ";
    }
    std::cout << std::endl;

    //now read configuration.txt
    string filename = "configuration.txt";

	ifstream f_in(filename);
	string line;


    hidden_layer_neurons = 0;

    string hidden_layer_weights = "Hidden layer ";
    hidden_layer_weights += to_string(hidden_layer_id);
    hidden_layer_weights += " weights";

    while (getline(f_in, line)) {
        if (line.find("layers ")!= string::npos){
            size_t index = line.find("layers ");
	        num_of_semaphores = line[index - 2]-48;
            num_of_semaphores--; //since always total layers - 1
        }
		else if (line.find(hidden_layer_weights) != string::npos) {
			getline(f_in, line);
            int i = 0;
            while(line[i]){
                if(line[i]==',') hidden_layer_neurons++;
                i++;
            }
            hidden_layer_neurons++;
		}
	}

    f_in.close();

    
    cout << "\nHidden layer neurons count = " << hidden_layer_neurons << endl;

    hidden_layer1.resize(hidden_layer_neurons);

    ifstream f_in2(filename);

    // read file again

    while (getline(f_in2, line)) {
		if (line.find(hidden_layer_weights) != string::npos) {
			for (int i = 0; i < hidden_layer_neurons; i++) {
				getline(f_in2, line);
				store_values(hidden_layer1[i], line);
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
    for (int i = 0; i < hidden_layer_neurons; i++) {
        ans[i] += params->input_layer[neuron_id][i] * neuron_val;
    }

	cout << "Calculation of one nueron is being done, threads are synchronised without using JOIN statements." << endl;

	cout << "Lock is unacquired." << endl;

	if (counter == hidden_layer_neurons) {
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
	ans += "\n";
    ans+=to_string(hidden_layer_id+1);

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

void write_data_into_pipe(char buffer[100]){

        /* open the pipe for writing */
        int fd = open(PIPE_NAME, O_WRONLY);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        else{
            cout << "\nPipe has been opened for writing. The process will now go in blocked state, unless reader process also activates." << endl;
        }
    
        /* write data to the pipe */
        if (write(fd, buffer, 100) == -1) {
        perror("write");
        exit(EXIT_FAILURE);
        }
        else{
            cout << "Data writen into pipe." << endl << endl;
        }
        /* close the pipe */
        close(fd);
}

void setting_up_pipe(){
     int result = remove(PIPE_NAME);
        if (result == 0) {
          //  printf("Named pipe '%s' has been successfully deleted.\n", PIPE_NAME);
        } else {
          //  printf("Error: could not delete named pipe '%s'.\n", PIPE_NAME);
        }

        /* create the named pipe */
        if (mkfifo(PIPE_NAME, 0666) == -1) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
        else{
           // cout << "Pipe has been created. " << endl;
        }

        if (access(PIPE_NAME, F_OK) == -1) {
        perror("access");
        exit(EXIT_FAILURE);
        }
}

void write_data_into_pipe_front(char* data_for_pipe, size_t size){
     /* open the pipe for writing */
        int fd = open(PIPE_NAME, O_WRONLY);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        else{
            cout << "Pipe has been opened for writing. The process will now go in blocked state, unless reader process also activates." << endl;
        }

        
        /* write data to the pipe */
        if (write(fd, data_for_pipe, size) == -1) {
        perror("write");
        exit(EXIT_FAILURE);
        }
        else{
            cout << "Data writen into pipe." << endl;
        }


        /* close the pipe */
        close(fd);
}

int setting_up_semaphores(){
    
    
    sems = new sem_t* [num_of_semaphores];
    for (int i = 0; i < num_of_semaphores; ++i) {
        sems[i] = new sem_t;
    }
    sem_names = new char*[num_of_semaphores];

    for(int i = 0;i<num_of_semaphores;i++){
       
        // Create semaphore name
        string name = "my_semaphore" + to_string(i);
		sem_names[i] = new char[name.length()+1]{};
        for(int j = 0;j<name.length();j++){
			sem_names[i][j] = name[j]; 
		}
        if(i==processss){
            // Open semaphore
            sems[i] = sem_open(sem_names[i], 0);
            if (sems[i] == SEM_FAILED) {
                perror("sem_open");
                return 1;
            }
            cout << sem_names[i] << " opening." << endl;
            // Save semaphore handle
            sems[i-1] = sem_open(sem_names[i-1], 0);
            if (sems[i-1] == SEM_FAILED) {
                perror("sem_open");
                return 1;
            }
            cout << sem_names[i-1] << "  opening." << endl;
        }
    }

    // if(processss==5){
    //     sem5 = sem_open(sem_name5, 0);
    //     if (sem5 == SEM_FAILED) {
    //         perror("sem_open");
    //         return 1;
    //     }
    //     sem4 = sem_open(sem_name4, 0);
    //     if (sem4 == SEM_FAILED) {
    //         perror("sem_open");
    //         return 1;
    //     }
    //  }
    //  else if (processss==4){
    //     sem4 = sem_open(sem_name4, 0);
    //     if (sem4 == SEM_FAILED) {
    //         perror("sem_open");
    //         return 1;
    //     }
    //     sem3 = sem_open(sem_name3, 0);
    //     if (sem3 == SEM_FAILED) {
    //         perror("sem_open");
    //         return 1;
    //     }
    //  }
    //  else if (processss==3){
    //     sem3 = sem_open(sem_name3, 0);
    //     if (sem3 == SEM_FAILED) {
    //         perror("sem_open");
    //         return 1;
    //     }
    //     sem2 = sem_open(sem_name2, 0);
    //     if (sem2 == SEM_FAILED) {
    //         perror("sem_open");
    //         return 1;
    //     }
    //  }
    // else if (processss==2){
    //     sem2 = sem_open(sem_name2, 0);
    //     if (sem2 == SEM_FAILED) {
    //         perror("sem_open");
    //         return 1;
    //     }
    //     sem1 = sem_open(sem_name1, 0);
    //     if (sem1 == SEM_FAILED) {
    //         perror("sem_open");
    //         return 1;
    //     }
    //  }
    // else if (processss==1){
    //     sem1 = sem_open(sem_name1, 0);
    //     if (sem1 == SEM_FAILED) {
    //         perror("sem_open");
    //         return 1;
    //     }
    //     sem = sem_open(sem_name, 0);
    //     if (sem == SEM_FAILED) {
    //         perror("sem_open");
    //         return 1;
    //     }
    //  }
     return 1;
}