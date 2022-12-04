#include "SC_PlugIn.h"
#include <dlfcn.h>
#include <string>
#include <iostream>
#define PI 3.141592653589793238462
#define MAX_CHANNELS 8

#define typeof(x) __typeof__(x)

#define RTALLOC_AND_CHECK(lhs, size)                       \
  (lhs) = (typeof(lhs))RTAlloc(unit->mWorld, (size));      \
  memset((lhs), 0, (size));                                \
  if (!(lhs))                                              \
  {                                                        \
    SETCALC(ClearUnitOutputs);                             \
    ClearUnitOutputs(unit, 1);                             \
    if (unit->mWorld->mVerbosity > -2)                     \
    {                                                      \
      Print("Failed to allocate memory for Dynlib ugen."); \
    }                                                      \
    return;                                                \
  }

// typedef void (*equation_def)(double X[], double t, double dX[], double params[]);

typedef void (*process_blockwise_def)(double **state_in, double **state_out, double **out, double **in, double params[], double dt, int blocksize);
typedef void (*process_def)(double **state_in, double **state_out, double out[], double in[], double params[], double dt);
typedef void (*setup_def)(double dt, int blocksize);
typedef void (*close_def)();

// InterfaceTable contains pointers to functions in the host (server).
static InterfaceTable *ft;

// declare struct to hold unit generator state
struct Dynlib : public Unit
{
  char *m_string;
  int m_string_size;
  unsigned int N_STATE, N_PARAMETERS, N_INPUTS, N_OUTPUTS, BLOCKWISE;
  int c = 0;
  bool ok = false;

  setup_def setup;
  close_def close;
  process_def process;
  process_blockwise_def process_blockwise;

  void *handle;

  double dt;
  double t;
  float f;
  int flag;

  // ARRAYS

  // N_INPUT/N_OUTPUT x N_STATE SIZE
  double **x_state_in;
  double **x_state_out;

  // N_PARAMETERS SIZE
  double *params;

  // inlets & outlets signals

  // N_INPUTS x N_BLOCK SIZE
  double **in;
  double **out;

  // N_INPUTS SIZE
  double *aux_in;

  // N_OUTPUTS SIZE
  double *aux_out;
};

// declare unit generator functions
static void Dynlib_next_a(Dynlib *unit, int inNumSamples);
// static void Dynlib_next_k(Dynlib *unit, int inNumSamples);
static void Dynlib_Ctor(Dynlib *unit);
static void Dynlib_Dtor(Dynlib *unit);

//////////////////////////////////////////////////////////////////

// Ctor is called to initialize the unit generator.
// It only executes once.

// A Ctor usually does 3 things.
// 1. set the calculation function.
// 2. initialize the unit generator state variables.
// 3. calculate one sample of output.
void Dynlib_Ctor(Dynlib *unit)
{
  printf("Dynlib v0.2\n");
  // printf("SAMPLEDUR %g",SAMPLEDUR);
  // printf("mBufLength %d",unit->mBufLength );
  // printf("calc_FullRate %d",calc_FullRate);

  // set the calculation function.
  SETCALC(Dynlib_next_a);

  unit->c = 0;
  unit->m_string_size = IN0(0); // number of chars in the id string
  unit->m_string = (char *)RTAlloc(unit->mWorld, (unit->m_string_size + 1) * sizeof(char));
  for (int i = 0; i < unit->m_string_size; i++)
  {
    unit->m_string[i] = (char)IN0(1 + i);
  };
  unit->m_string[unit->m_string_size] = 0; // terminate string
  std::string plugin_name(unit->m_string);
  // Print("Ode name %s",unit->m_string);
  // Print("Ode name %s",plugin_name.c_str());
  std::string libname("./lib" + plugin_name + ".so");
  Print("Plugin name: %s\n", plugin_name.c_str());
  Print("Lib name: %s\n", libname.c_str());
  unit->handle = dlopen(libname.c_str(), RTLD_LAZY);

  if (unit->handle != NULL)
  {
    Print("%s: DLOPEN: ok\n", unit->m_string);

    int sett[5];

    void (*settings)(int *);

    settings = (void (*)(int *))dlsym(unit->handle, "settings");
    unit->setup = (setup_def)dlsym(unit->handle, "setup");
    unit->close = (close_def)dlsym(unit->handle, "close");
    if (unit->BLOCKWISE)
      unit->process_blockwise = (process_blockwise_def)dlsym(unit->handle, "process_blockwise");
    else
      unit->process = (process_def)dlsym(unit->handle, "process");

    // void (*dimensions)(int *);
    // dimensions = (void (*)(int *))dlsym(unit->handle, "dimensions");
    // int dims[2];
    // dimensions(dims);
    // unit->equation = (equation_def)dlsym(unit->handle, "equation");
    settings(sett);

    unit->N_STATE = sett[0];
    unit->N_OUTPUTS = sett[1];
    unit->N_INPUTS = sett[2];
    unit->N_PARAMETERS = sett[3];
    unit->BLOCKWISE = sett[4];

    Print("%s: N_STATE=%d\t N_OUTPUTS=%d\t N_INPUTS=%d\t N_PARAMETERS=%d\t BLOCKWISE=%d\n",
          unit->m_string, unit->N_STATE, unit->N_OUTPUTS, unit->N_INPUTS, unit->N_PARAMETERS, unit->BLOCKWISE);

    RTALLOC_AND_CHECK(unit->in, unit->N_INPUTS * sizeof(double));
    RTALLOC_AND_CHECK(unit->out, unit->N_OUTPUTS * sizeof(double));
    RTALLOC_AND_CHECK(unit->aux_in, unit->N_INPUTS * sizeof(double));
    RTALLOC_AND_CHECK(unit->aux_out, unit->N_OUTPUTS * sizeof(double));
    RTALLOC_AND_CHECK(unit->x_state_in, unit->N_INPUTS * sizeof(double *));
    RTALLOC_AND_CHECK(unit->x_state_out, unit->N_OUTPUTS * sizeof(double *));
    int N = 64;

    for (int k = 0; k < unit->N_INPUTS; k++)
    {
      RTALLOC_AND_CHECK(unit->in[k], N * sizeof(double));
    }

    for (int k = 0; k < unit->N_OUTPUTS; k++)
    {
      RTALLOC_AND_CHECK(unit->out[k], N * sizeof(double));
    }

    for (int k = 0; k < unit->N_INPUTS; k++)
    {
      RTALLOC_AND_CHECK(unit->x_state_in[k], unit->N_STATE * sizeof(double));
    }

    for (int k = 0; k < unit->N_OUTPUTS; k++)
    {
      RTALLOC_AND_CHECK(unit->x_state_out[k], unit->N_STATE * sizeof(double));
    }

    RTALLOC_AND_CHECK(unit->params, unit->N_PARAMETERS * sizeof(double));

    Print("RTALLOCS\n");

    unit->dt = SAMPLEDUR;
    unit->t = 0;

    for (int k = 0; k < unit->N_INPUTS; k++)
    {
      for (int i = 0; i < N; ++i)
        unit->in[k][i] = IN(unit->m_string_size + 1 + k)[i];

      Print("%s: IN %g\n", unit->m_string, unit->in[k][0]);
    }

    for (int k = 0; k < unit->N_PARAMETERS; k++)
    {
      unit->params[k] = IN0(unit->m_string_size + 1 + k + unit->N_INPUTS);
      Print("%s: PARAMS %g\n", unit->m_string, unit->params[k]);
    }

    for (int k = 0; k < unit->N_OUTPUTS; k++)
    {
      OUT0(k) = unit->out[k][0];
      Print("%s: OUT %g\n", unit->m_string, unit->out[k][0]);
    }
    unit->ok = true;

    Print("SETUP\n");

    unit->setup(unit->dt, N);

    Print("CALL\n");

    unit->process_blockwise(unit->x_state_in, unit->x_state_out, unit->in, unit->out, unit->params, unit->dt, N);
  }
  else
  {
    unit->ok = false;
    SETCALC(ClearUnitOutputs);
    ClearUnitOutputs(unit, 1);
    Print("%s: DLOPEN: not ok\n", unit->m_string);
  }
}

void Dynlib_Dtor(Dynlib *unit)
{
  if (unit->ok)
  {

    RTFree(unit->mWorld, unit->in);
    RTFree(unit->mWorld, unit->out);
    for (unsigned int i = 0; i < unit->N_INPUTS; i++)
      RTFree(unit->mWorld, unit->in[i]);

    for (unsigned int i = 0; i < unit->N_OUTPUTS; i++)
      RTFree(unit->mWorld, unit->out[i]);

    RTFree(unit->mWorld, unit->x_state_in);
    RTFree(unit->mWorld, unit->x_state_out);

    for (unsigned int i = 0; i < unit->N_INPUTS; i++)
      RTFree(unit->mWorld, unit->x_state_in[i]);

    for (unsigned int i = 0; i < unit->N_OUTPUTS; i++)
      RTFree(unit->mWorld, unit->x_state_out[i]);

    RTFree(unit->mWorld, unit->params);
    RTFree(unit->mWorld, unit->aux_in);
    RTFree(unit->mWorld, unit->aux_out);
    Print("%s: All Free", unit->m_string);

    unit->close();

    dlclose(unit->handle);
    Print("%s: Closed", unit->m_string);
    RTFree(unit->mWorld, unit->m_string);
  }
}

//////////////////////////////////////////////////////////////////

// The calculation function executes once per control period
// which is typically 64 samples.

// calculation function for an audio rate frequency argument

void Dynlib_next_a(Dynlib *unit, int inNumSamples)
{

  for (int k = 0; k < unit->N_PARAMETERS; k++)
  {
    unit->params[k] = IN0(unit->m_string_size + 1 + k + unit->N_INPUTS);
  }

  for (int k = 0; k < unit->N_INPUTS; k++)
  {
    for (int i = 0; i < inNumSamples; ++i)
    {
      unit->in[k][i] = IN(unit->m_string_size + 1 + k)[i];
    }
  }

  unit->process_blockwise(unit->x_state_in, unit->x_state_out, unit->in, unit->out, unit->params, unit->dt, inNumSamples);

  for (int k = 0; k < unit->N_OUTPUTS; k++)
  {
    for (int i = 0; i < inNumSamples; ++i)
    {
      // Print("%g ", zapgremlins(unit->out[k][i]));

      OUT(k)
      [i] = zapgremlins(unit->out[k][i]);
    }
  }

  for (unsigned int i = 0; i < unit->N_INPUTS; i++)
    for (unsigned int j = 0; j < unit->N_STATE; j++)
      unit->x_state_in[i][j] = zapgremlins(unit->in[i][inNumSamples - 1 - j]);

  for (unsigned int i = 0; i < unit->N_OUTPUTS; i++)
    for (unsigned int j = 0; j < unit->N_STATE; j++)
      unit->x_state_out[i][j] = zapgremlins(unit->out[i][inNumSamples - 1 - j]);
}

//////////////////////////////////////////////////////////////////

// void Dynlib_next_k(Dynlib *unit, int inNumSamples)
// {

//     for (int i=0; i < inNumSamples; ++i)
//     {

//         for(int k=0;k<unit->N_PARAMETERS;k++)
//         {
//             unit->params[k] = IN0(k);
//         }

//         rk4( unit );

//         for(int k=0;k<unit->N_EQ;k++)
//         {
//             OUT(k)[i] = unit->X[k];
//         }
//     }

// }

// the entry point is called by the host when the plug-in is loaded
PluginLoad(Dynlib)
{
  // InterfaceTable *inTable implicitly given as argument to the load function
  ft = inTable; // store pointer to InterfaceTable

  DefineDtorUnit(Dynlib);
}
