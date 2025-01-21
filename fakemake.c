/*Thanya Nguyen Lab 4
  2/21/2024 - This lab makes a mock-up makefile.
  Help from TA Maria and Adam and jacob!*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fields.h"
#include "dllist.h"
#include "jrb.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

long long int max_header_time(Dllist h_list, struct stat buf, Dllist tmp){
	int exists;
	long long int max = 0;
	//printf("doing thru the headers and finding the max??\n");
	dll_traverse(tmp, h_list){ //traversing thru the tree and calling stat on each file
		exists = stat(tmp->val.s, &buf);
		if(exists < 0){
			printf("header DNE - %s\n", tmp->val.s);
			return -1;
		}
		else {
			if(buf.st_mtime > max) max = buf.st_mtime;
			//printf("%10lld %s\n", buf.st_mtime, tmp->val.s);
		}
	}
	return max;
}

void remove_all(IS is,char *executable, Dllist c_list, Dllist h_list,Dllist l_list,Dllist f_list, Dllist o_list){
	Dllist tmp;

	dll_traverse(tmp, c_list) free(tmp->val.s);
	dll_traverse(tmp, h_list) free(tmp->val.s);
	dll_traverse(tmp, l_list) free(tmp->val.s); 
	dll_traverse(tmp, f_list) free(tmp->val.s); 
	dll_traverse(tmp, o_list) free(tmp->val.s);
	free(executable);

	free_dllist(c_list); 
	free_dllist(h_list); 
	free_dllist(l_list); 
	free_dllist(f_list); 
	free_dllist(o_list); 

	jettison_inputstruct(is);

}

int main(int argc, char **argv) {
	IS is;
	FILE *f, *read_file;
	Dllist c_list, h_list, l_list, f_list, o_list, tmp;
	char *filename;
	char o_cmd[10000]= {0};
	char cmd[10000]= {0};
	struct stat buf;
	int i, exists;
	char *executable;
	long long int header_time, object_time;
	executable = NULL;
	int line_counter = 0;

	// Determine the filename to use
	filename = (argc > 1) ? argv[1] : "fmakefile"; //teriary expression ftw
	is = new_inputstruct(filename);
	if (is == NULL) {
		// Print an error message specific to the problem encountered
		fprintf(stderr, "Error opening file: %s\n", filename);
		remove_all(is, executable,c_list, h_list,l_list, f_list, o_list);
		return -1;
	}

	/*girl why??*/
	//fprintf(read_file, "hello kitty test!\n"); //testing
	c_list = new_dllist();
	h_list = new_dllist();
	l_list = new_dllist();
	f_list = new_dllist();
	o_list = new_dllist();

	while(get_line(is) >= 0) {
		line_counter++;
		if (is->NF <= 0) {
			printf("blank line \n");
		}

		if (strcmp(is->fields[0], "C") == 0) { //if its a compile line
			//printf("C: %s", is->text1);
			for(i = 1; i < is->NF;i++){
				//printf("is->fields[%d]: %s\n", i, is->fields[i]);
				dll_append(c_list, new_jval_s(strdup(is->fields[i]))); //free this later
			}
		}
		else if(strcmp(is->fields[0], "H") == 0){ //h line case
			//printf("H: %s", is->text1);
			dll_append(h_list, new_jval_s(strdup(is->fields[1])));
		}
		else if(strcmp(is->fields[0], "L") == 0){ //h line case
			//printf("L: %s", is->text1);
			for(i = 1; i < is->NF;i++){
				dll_append(l_list, new_jval_s(strdup(is->fields[i])));
			}
		}
		else if(strcmp(is->fields[0], "E") == 0){
			if(executable != NULL){
				fprintf(stderr, "fmakefile (%d) cannot have more than one E line\n", line_counter);
				remove_all(is, executable,c_list, h_list,l_list, f_list, o_list);
				return -1;
			}
			//printf("   executable found !\n");
			executable = strdup(is->fields[1]);
			//printf("executable is %s\n", strdup(is->fields[1]));
		}
		else if(strcmp(is->fields[0], "F") == 0){ //h line case
			//printf("F: %s", is->text1);
			for(i = 1; i < is->NF;i++){
				dll_append(f_list, new_jval_s(strdup(is->fields[i])));
			}
		}

	} 

	if (executable == NULL){
		fprintf(stderr, "No executable specified\n");
		remove_all(is, executable,c_list, h_list,l_list, f_list, o_list);
		return -1;
	}

	header_time = max_header_time(h_list, buf, tmp);
	if(header_time == -1){ /*cannot print out this error in stdout*/
		fprintf(stderr, "1Command failed.  Fakemake exiting\n");
		remove_all(is, executable,c_list, h_list,l_list, f_list, o_list);
		return -1;
	}

	/*traversing the flags and concatenating them so for the object compile line*/
	char all_flags[10000]= {0};
	dll_traverse(tmp, f_list){
		strcat(all_flags, " ");
		strcat(all_flags, tmp->val.s);

	}

	struct stat o_buf;
	struct stat c_buf;
	char o_filename[256]= {0};
	int o_system;
	object_time = 0;
	int run_again = 0;

	dll_traverse(tmp, c_list){
		exists = stat(tmp->val.s, &c_buf);//calls stat
		if(exists < 0){
			fprintf(stderr, "fmakefile: %s: No such file or directory\n", tmp->val.s);
			remove_all(is, executable,c_list, h_list,l_list, f_list, o_list);
			return -1;
		}
		else {
			strcpy(o_filename, tmp->val.s); 
			o_filename[strlen(o_filename) - 1] = 'o';//changing it to a .o file
			dll_append(o_list, new_jval_s(strdup(o_filename)));
			int o_exists = stat(o_filename, &o_buf);

			// if it DNE       if .o time < .c time            if .o time < max_headertime
			if (o_exists < 0 || o_buf.st_mtime < c_buf.st_mtime || o_buf.st_mtime < header_time) {
				//printf("run again!\n");
				/*creating object file here*/
				run_again = 1; // now i have to remake executable later

				sprintf(o_cmd, "gcc -c%s %s", all_flags, tmp->val.s); //make a function that traverses and concatenates all of them??
				printf("%s\n", o_cmd);
				o_system = system(o_cmd);
				if(o_system != 0){ //might be 0
					fprintf(stderr, "Command failed.  Exiting\n");
					remove_all(is, executable,c_list, h_list,l_list, f_list, o_list);
					return -1;
				}

			} else {
				if(o_buf.st_mtime > object_time) object_time = o_buf.st_mtime; //maintains max .o time
			}
		}
	}
	//concatenating all objects
	char all_objects[10000]= {0};;
	dll_traverse(tmp, o_list){
		strcat(all_objects, " ");
		strcat(all_objects, tmp->val.s);
	}
	/*concatenating all libs*/
	char all_libs[10000] = {0};
	dll_traverse(tmp, l_list){
		strcat(all_libs, " ");
		strcat(all_libs, tmp->val.s);
	}

	/*another check, i think i missed it 
	but i am testinf if ht eexecutable exists*/
	struct stat e_buf;
	int e_exists;
	char exec_name[10000] = {0};
	e_exists = stat(executable, &e_buf);
	if(e_exists < 0 || e_buf.st_mtime < object_time){
		run_again = 1;
	}

	/*error checks*/
	if(run_again == 0) printf("%s up to date\n", executable);
	if (executable == NULL)printf("exectable is NULL\n");


	int check;
	/*if you need to recompile it*/
	if(run_again == 1){
		sprintf(cmd, "gcc -o %s%s%s%s", executable, all_flags ,all_objects, all_libs); //make a function that traverses and concatenates all of them??
		printf("%s\n", cmd);
		check = system(cmd);
		if(check != 0){ //might be != 0
			fprintf(stderr, "Command failed.  Fakemake exiting\n");
			remove_all(is, executable,c_list, h_list,l_list, f_list, o_list);
			return -1;
		}
	}

	remove_all(is, executable,c_list, h_list,l_list, f_list, o_list);
	return 0;
}
