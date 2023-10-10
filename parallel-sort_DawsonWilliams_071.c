#define _GNU_SOURCE

#include <stdio.h>
#include <pthread.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

typedef struct //parameter struct for qsort_r
{
int *pData;
int subArrayLen;
int threadNum;
int nmemb;
} parameters;

typedef struct //parameters for merge
{
int *a;
int left;
int middle;
int right;
int *tmp;
int i;
int j;
}m_parameters;

int cmpintp(const void *p1, const void *p2, void *p3){ 
    int v1 = *((int *)p1);
    int v2 = *((int *)p2);
    return (v1 - v2);
}

void merge(int a[], int left, int middle, int right, int tmp[]){ 
    int i = left, j = middle, k = left;
    while (i < middle || j < right) {
        if (i < middle && j < right)
        { // Both array have elements
            if (a[i] < a[j])
                tmp[k++] = a[i++];
            else
                tmp[k++] = a[j++];
        } else if (i == middle)
            tmp[k++] = a[j++]; // a is empty
        else if (j == right)
            tmp[k++] = a[i++]; // b is empty
    }
    for (i = left; i < right; i++)
        a[i] = tmp[i]; /* copy tmp[] back to a[] */
}

void *qsort_helper(void *param) {
    //convert back to a parameter pointer
    parameters *p = (parameters *)param;
    //qsort_r with given paramaters 
    qsort_r(p->pData, p->subArrayLen, sizeof(int), cmpintp, NULL);
    //print info of the thread
    printf("sort[%d]: base=%p, nmemb=%d, size=%ld\n",p->threadNum, &p->pData, p->nmemb, sizeof(int));
    //dealloc dynamic parameter pointer
    free(param);
}

void *merge_helper(void *m_param){
    //convert back to m_parameter pointer
    m_parameters *p = (m_parameters *)m_param;
    //call merge with given parameters
    merge(p->a, p->left, p->middle, p->right, p->tmp);
    //print info of the thread
    printf("merge[%d, %d]: base=%p, left=%d, middle=%d, right=%d\n",p->i, p->j, &p->a, p->left, p->middle, p->right);
    //dealloc dynamic tmp array and the dynamic m_parameter struct pointer
    free(p->tmp);
    free(m_param);
}

int main(int argc, char *args[]){
    printf("CS149 Spring 2023 parallel sort by Dawson Williams\n");
    //error check input parameters
    int length = atoi(args[1]); //total amount of numbers in array 32
    int parts = atoi(args[2]); //initial num of partitions 8
    if(length < 0 || parts < 0){
        printf("Negative digit\n");
        return -1;
    }
    if(length == 0 || parts == 0){
        printf("Zero digit\n");
        return -1;
    }
    if(length % parts != 0){
        printf("Not divisible\n");
        return -1;
    }

    int pLength = length/parts; //length of each initial partition 4
    //2^n = 8
    //n = lg(8)
    //n is num of merges total
    if(log2(parts) != floor(log2(parts))){
        //error
        printf("it is not a base of 2\n");
        return -1;
    }
    int n = log2(parts);
    //create array
    int* arr = (int *)malloc(sizeof(int) * length);
    unsigned int seed = time(NULL); //use time as seed to get different values every run
    for(int i = 0; i < length; i++){
        arr[i] = rand_r(&seed) % 10000;
    }

    printf("Before: ");
    for(int i = 0; i < length; i++){
        printf("%d ", arr[i]);
    }
    printf("\n");
    //single parititon becomes immediately sorted
    if(parts == 1){
        qsort_r(arr, length, sizeof(int), cmpintp, NULL);
        printf("After: ");
        for(int i = 0; i < length; i++){
            printf("%d ", arr[i]);
        }
        printf("\n\n");
    }
    else{
        pthread_t id[parts]; //create num of threads equal to partitions
        for(int j = 0; j < parts; j++){
            parameters *pParam = (parameters *) malloc(sizeof(parameters));
            pParam->pData = (arr + (j*pLength)); //will point to every unit of length per thread
            pParam->subArrayLen = pLength; //length of partitions
            pParam->threadNum = j; //thread number
            pParam->nmemb = pLength; //length of partition 
            if(pthread_create(&id[j], NULL, qsort_helper, (void *) pParam) != 0){
                perror("Failed to create threads.\n");
                return 1;
            }
        }
        //join back threads after they have been created and executed
        for(int j = 0; j < parts; j++){
            if(pthread_join(id[j], NULL) != 0){
                perror("Failed to join threads\n");
                return 1;
            }   
        }
        //start phase 2
        for(int i = 0; i < n; i++){ //for every iteration
            int numOfThreads = parts / 2; //num of threads halves from each ittr
            pthread_t id[numOfThreads]; //create array for thread ids
            for(int j = 0; j < numOfThreads; j++){
                m_parameters *pParam = (m_parameters *) malloc(sizeof(m_parameters)); //create param struct pointer
                int left, middle, right;
                int * tmp = (int *)malloc(sizeof(int) * length); //create dynamic tmp arr, otherwise error "stack smashing"
                left = j * pLength * 2; //left will skip to front of every 2 subarrays
                middle = left + pLength; //middle is one partition more of the left
                right = middle + pLength; //right is one partition more of the middle
                //** Init all merge parameters, as well as other useful info
                pParam -> left = left;
                pParam -> middle = middle;
                pParam -> right = right;
                pParam -> a = arr;
                pParam -> tmp = tmp;
                pParam -> i = i;
                pParam -> j = j;
                if(pthread_create(&id[j], NULL, merge_helper, (void *) pParam) != 0){
                    perror("Failed to create threads.\n");
                    return 1;
                }
            }

            for(int j = 0; j < numOfThreads; j++){
                if(pthread_join(id[j], NULL) != 0){
                   perror("Failed to join threads\n");
                   return 1;
               }
            }

            parts = parts / 2; //half the num of parititions
            pLength = pLength * 2; //length of paritions double
        }

        printf("After: ");
        for(int i = 0; i < length; i++){
            printf("%d ", arr[i]);
        }
        printf("\n\n");
        
    }
    return 0;
}