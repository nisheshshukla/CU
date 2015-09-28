#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include "Filter.h"


using namespace std;

#include "rtdsc.h"

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int
main(int argc, char **argv)
{

  if ( argc < 2) {
    fprintf(stderr,"Usage: %s filter inputfile1 inputfile2 .... \n", argv[0]);
  }

  //
  // Convert to C++ strings to simplify manipulation
  //
  string filtername = argv[1];

  //
  // remove any ".filter" in the filtername
  //
  string filterOutputName = filtername;
  string::size_type loc = filterOutputName.find(".filter");
  if (loc != string::npos) {
    //
    // Remove the ".filter" name, which should occur on all the provided filters
    //
    filterOutputName = filtername.substr(0, loc);
  }

  Filter *filter = readFilter(filtername);

  double sum = 0.0;
  int samples = 0;

  for (int inNum = 2; inNum < argc; inNum++) {
    string inputFilename = argv[inNum];
    string outputFilename = "filtered-" + filterOutputName + "-" + inputFilename;
    struct cs1300bmp *input = new struct cs1300bmp;
    struct cs1300bmp *output = new struct cs1300bmp;
    int ok = cs1300bmp_readfile( (char *) inputFilename.c_str(), input);

    if ( ok ) {
      double sample = applyFilter(filter, input, output);
      sum += sample;
      samples++;
      cs1300bmp_writefile((char *) outputFilename.c_str(), output);
    }
    delete input;
    delete output;
  }
  fprintf(stdout, "Average cycles per sample is %f\n", sum / samples);

}

struct Filter *
readFilter(string filename)
{
  ifstream input(filename.c_str());

  if ( ! input.bad() ) {
    int size = 0;
    input >> size;
    Filter *filter = new Filter(size);
    int div;
    input >> div;
    filter -> setDivisor(div);
    for (int i=0; i < size; i++) {
      for (int j=0; j < size; j++) {
  int value;
  input >> value;
  filter -> set(i,j,value);
      }
    }
    return filter;
  }
}

double
applyFilter(struct Filter *filter, cs1300bmp *input, cs1300bmp *output)
{


  long long cycStart, cycStop;

  cycStart = rdtscll();

  int h = input -> height - 1;
  int w = input -> width - 1;

  output -> width = w + 1;
  output -> height = h + 1;


  int filter_array[3][3];
  
  for (int i = 0; i < 3; i++){
    filter_array[i][0] = filter -> get(i,0);
    filter_array[i][1] = filter -> get(i,1);
    filter_array[i][2] = filter -> get(i,2);
  }

  // Hi-Line
  if (filter_array[0][1] == -2){
    
    for(int p = 0; p < 3; p++) {
     
      for(int r = 1; r < h; r++) {

        int new_row_1 = r - 1;
        int new_row_2 = r + 1;

        for(int c = 1; c < w; c++) {

          int temp_1=0, temp_2=0, temp_3=0;
          int col_1 = c - 1;
          int col_2 = c + 1;

          temp_1 += -(input->color[p][new_row_1][col_1]);
          temp_2 += -(input->color[p][new_row_1][c] << 1);
          temp_3 += -(input->color[p][new_row_1][col_2]);

          temp_1 += input->color[p][new_row_2][col_1];
          temp_2 += input->color[p][new_row_2][c] << 1;
          temp_3 += input->color[p][new_row_2][col_2];
        
          temp_1 = temp_1+temp_2+temp_3;

          if (temp_1 > 255){
            output->color[p][r][c] = 255;
            continue;
          }
          if (temp_1 < 0){
            output->color[p][r][c] = 0;
            continue;
          }

          output->color[p][r][c] = temp_1;
        }
      }
    }
  }

  // Gauss
  else if (filter_array[1][1] == 8){
    
    for(int p = 0; p < 3; p++) {
    
      for(int r = 1; r < h; r++) {

        int new_row_1 = r - 1;
        int new_row_2 = r + 1;

        for(int c = 1; c < w; c++) {

          int temp_1=0, temp_2=0, temp_3=0;
          int col_1 = c - 1;
          int col_2 = c + 1;

          temp_2 += input->color[p][new_row_1][c] << 2;

          temp_1 += input->color[p][r][col_1] << 2;
          temp_2 += input->color[p][r][c] << 3;
          temp_3 += input->color[p][r][col_2] << 2;

          temp_2 += input->color[p][new_row_2][c] << 2;
          
          // divide by 24
          temp_1 = ((temp_1+temp_2+temp_3) >> 3) / 3;

          if (temp_1 > 255){
            output->color[p][r][c] = 255;
            continue;
          }
          if (temp_1 < 0){
            output->color[p][r][c] = 0;
            continue;
          }

          output->color[p][r][c] = temp_1;
        }
      }
    }
  }

  // Emboss
  else if (filter_array[1][2] == -1) {
    
    for(int p = 0; p < 3; p++) {
    
      for(int r = 1; r < h; r++) {

        int new_row_1 = r - 1;
        int new_row_2 = r + 1;

        for(int c = 1; c < w; c++) {

          int temp_1=0, temp_2=0, temp_3=0;
          int col_1 = c - 1;
          int col_2 = c + 1;

          temp_1 += input->color[p][new_row_1][col_1];
          temp_2 += input->color[p][new_row_1][c];
          temp_3 += -(input->color[p][new_row_2][c]);

          temp_1 += input->color[p][r][col_1];
          temp_2 += input->color[p][r][c];
          temp_3 += -(input->color[p][new_row_2][c]);

          temp_1 += input->color[p][new_row_2][col_1];
          temp_2 += -(input->color[p][new_row_2][c]);
          temp_3 += -(input->color[p][new_row_2][col_2]);
          
          temp_1 += temp_2+temp_3;

          if (temp_1 > 255){
            output->color[p][r][c] = 255;
            continue;
          }
          if (temp_1 < 0){
            output->color[p][r][c] = 0;
            continue;
          }

          output->color[p][r][c] = temp_1;
        }
      }
    }
  }
  // Average
  else{
    
    for(int p = 0; p < 3; p++) {
    
      for(int r = 1; r < h; r++) {

        int new_row_1 = r - 1;
        int new_row_2 = r + 1;

        for(int c = 1; c < w; c++) {

          int temp_1=0, temp_2=0, temp_3=0;
          int col_1 = c - 1;
          int col_2 = c + 1;

          temp_1 += input->color[p][new_row_1][col_1];
          temp_2 += input->color[p][new_row_1][c];
          temp_3 += input->color[p][new_row_1][col_2];

          temp_1 += input->color[p][r][col_1];
          temp_2 += input->color[p][r][c];
          temp_3 += input->color[p][r][col_2];

          temp_1 += input->color[p][new_row_2][col_1];
          temp_2 += input->color[p][new_row_2][c];
          temp_3 += input->color[p][new_row_2][col_2];
    
          temp_1 = (temp_1+temp_2+temp_3) / 9;

          if (temp_1 < 0){
            output->color[p][r][c] = 0;
            continue;
          }

          if (temp_1 > 255){
            output->color[p][r][c] = 255;
            continue;
          }
          output->color[p][r][c] = temp_1;
        }
      }
    }
  }


  cycStop = rdtscll();
  double diff = cycStop - cycStart;
  double diffPerPixel = diff / (output -> width * output -> height);
  fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
    diff, diff / (output -> width * output -> height));
  return diffPerPixel;
}
