#include <math.h>
#include <iostream>
#define PI 3.141592653589793238462

#define N_STATE 0
#define N_INPUTS 0
#define N_OUTPUTS 2
#define N_PARAMETERS 1

#define BLOCKWISE 1

extern "C"
{
    double ph=0;

    void settings(int x[])
    {
        x[0] = N_STATE;
        x[1] = N_OUTPUTS;
        x[2] = N_INPUTS;
        x[3] = N_PARAMETERS;
        x[4] = BLOCKWISE;
    }

    void setup(double dt, int N)
    {
        std::cout << "DemoSin setup\\n" << std::endl;
        ph = 0;
    }

    void process_blockwise(double **state_in, double **state_out, double **in, double **out, double *param, double dt, int N)
    {
        
        float f = param[0];        
        float dph = 2*PI*f*dt;
        for (int i = 0; i < N; i++)
        {
            ph += dph;
            out[0][i] = sin(ph);
            out[1][i] = sin(ph);
        } 
    }
}