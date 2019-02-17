#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define min(x, y) ((x)<(y)?(x):(y))

//********************************
//   mpi matrix multiplication
//       Andrew D. Pitt
//          CIS 3238
//Current version multiplies ints instead of doubles, so no SIMD exploitation
//too many processes (more than nrows+1) results in infinte blank loop(waiting
//this is spaghetti code from drafting. i would like to revise this ASAP

int j=0, h = 0, k = 0; int lastOpen = 0;
int *recv2;

int numprocs, numsent, sender, anstype, row;//should numprocs always match matr$
double starttime, endtime;//add this slowly. may cause segfault

int* malloc_matrix(int n, int m) {
  int *z =(int*) malloc(sizeof(int) * n * m);
  return z;
}

int main(int argc, char *argv[]) {
  FILE *fp,*fp2;
  char buff[255];
  int *a, *b, *recv;
  int rank, i, nrows;
  int ncols = 0;
  MPI_Status status;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if((fp = fopen(argv[1], "r"))==NULL){
      fprintf(stderr,"Error opening file %s\n",argv[1]);
      exit(-1);
  }
  if((fp2 = fopen(argv[2], "r"))==NULL){
      fprintf(stderr,"Error opening file %s\n",argv[2]);
      exit(-1);
  }
//begin count columns routine
  fgets(buff,255,(FILE*)fp);//get first line 
  while(buff[i] != '\n'){ //using buff to count . to judge n cols
    if(buff[i]=='.')
      ncols++;
    i++;
   }
  nrows=ncols;//parameters for creating nxn array from this matrix
  rewind(fp);//set back to beginning
  //end count columns routine

  a = (malloc_matrix(nrows,ncols));
  b = (malloc_matrix(nrows,ncols));
  recv = (malloc_matrix(nrows,ncols));
  recv2 = malloc(nrows*sizeof(int));
  if (rank == 0){
    fgets(buff,255,(FILE*)fp);//get first line, sto->buff
    *a = atoi(strtok(buff,"."));//get just the first digit, tokenize on .
  //printf("%d ",*a);
  a++;//move the pointer
  int count = 0;
  while(count <= ncols-2){  //go through the rest of the line and add it to a
      *a = atoi(strtok(NULL,"."));//next token
      //printf("%d ",*a);
      a++; count++;//move a and counter
  }
while(fgets(buff,255,(FILE*)fp)!=NULL){//get next line in file
    *a = atoi(strtok(buff,"."));//get the first token
  //printf("%d ",*a);
  a++;
  count = 0;
  while(count <= ncols-2){//get the remaining tokens
      *a = atoi(strtok(NULL,"."));
      //printf("\n%d ",*a);
      a++; count++;
  }

  }//loop while file is not null
fgets(buff,255,(FILE*)fp2);//get first line, sto->buff

//begin count columns routine

  *b = atoi(strtok(buff,"."));//get just the first digit, tokenize on .
  //printf("%d ",*a);
  b++;//move the pointer
  count = 0;
  while(count <= ncols-2){  //go through the rest of the line and add it to a
      *b = atoi(strtok(NULL,"."));//next token
      //printf("%d ",*a);
      b++; count++;//move a and counter
  }
  while(fgets(buff,255,(FILE*)fp2)!=NULL){//get next line in file

  *b = atoi(strtok(buff,"."));//get the first token
  //printf("%d ",*a);
  b++;
  count = 0;
  while(count <= ncols-2){//get the remaining tokens
      *b = atoi(strtok(NULL,"."));
      //printf("\n%d ",*a);
      b++; count++;
  }

  }//loop while file is not null

  for(k=0;k<=((nrows*ncols)-1);k++) //rewind a to get ready to send over
     a--;

 for(k=0;k<=((nrows*ncols)-1);k++) //rewind a to get ready to send over
     b--; //printf("%d\n",*b);
     printf("\nMultiplying matrices\n\nMatrix A:\n");
  for(k=0; k<= ((nrows*ncols)-1);k++){
        printf("%d ",a[k]);
        if((k+1)%ncols==0)
          printf("\n");
  }
  printf("Matrix B\n");
  for(k=0; k<= ((nrows*ncols)-1);k++){
        printf("%d ",b[k]);
        if((k+1)%ncols==0)
          printf("\n");
  }

    starttime = MPI_Wtime();
                             //-2 for index is to offset h and ignore master
   for(h=0;h<=numprocs-2;h++)//send b to every process
     MPI_Send(b, nrows*ncols, MPI_INT, h+1, 0, MPI_COMM_WORLD);

  numsent = 0;
for (i = 0; i < min(numprocs-1, nrows); i++) {//row by row distribute a
    for (j = 0; j < ncols; j++) {//item in row
      buff[j] = a[i * ncols + j];//fill buffer with row
    }
    MPI_Send(buff, ncols, MPI_INT, i+1, i+1, MPI_COMM_WORLD);//send the row
    numsent++;
  }
  for (i = 0; i < nrows; i++) {//receive computed row*cols
//prev arg4 MPI_ANY_SOURCE or i+1
    MPI_Recv(recv2, ncols, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,//receive 1 int$
             MPI_COMM_WORLD, &status);
//    for(j=0;j<nrows;j++)
//      printf("recv2[j]: %d\n",recv2[j]);
    sender = status.MPI_SOURCE;
    anstype = status.MPI_TAG;
    for(j=0; j < nrows; j++){//has 1 be just n because recv is different every
        recv[lastOpen] = recv2[j];                                    //time
//      printf("recv[lastOpen]: %d\n",recv[lastOpen]);
        lastOpen++;
    }
// for(k=0;k<=24;k++){ //rewind a to get ready to send over
  //   recv--; //printf("%d\n",*b);

      if (numsent < nrows) {//if theres any more rows to send (if nprocs<nrows)
        for (j = 0; j < ncols; j++)
          buff[j] = a[numsent*ncols + j];
        MPI_Send(buff, ncols, MPI_INT, sender, numsent+1,
               MPI_COMM_WORLD);
numsent++;
    }
       else {
         MPI_Send(MPI_BOTTOM, 0, MPI_INT, sender, 0, MPI_COMM_WORLD);
       }
  } //end for waiting recieve/if more to send then send them

//    for(k=0;k<=((nrows*ncols)-1);k++)
//      printf("recv: %d\n",recv[k]);
  printf("Result:\n");
  for(k=0; k<= ((nrows*ncols)-1);k++){
        printf("%d ",recv[k]);
        if((k+1)%ncols==0)
          printf("\n");
  }
      endtime = MPI_Wtime();
      printf("\ntime: %f\n",(endtime - starttime));
    free(a); free(b);
  }
  else{
    MPI_Recv(b, nrows*ncols, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    while(1){
      MPI_Recv(buff, ncols, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if(status.MPI_TAG == 0)
            break;

      row = status.MPI_TAG;

      for(k=0; k<nrows*ncols; k++)
        recv[k] = 0;
      for(i = 0;i < ncols;i++){//go through all columns
        for(j=0;j<nrows;j++){
          recv[i] = recv[i] + (buff[j] * b[j*ncols+i]); //multiply 1 row member$
        }//end inner for
      }//end outer for
    MPI_Send(recv, ncols, MPI_INT, 0, row, MPI_COMM_WORLD);//send row back
    }//end infinte while

    }//end slave else

  MPI_Finalize();
  free(recv); //free(recv2);
  return 0;
}

