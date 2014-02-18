#ifndef __GADGETFILEHEADER
#define __GADGETFILEHEADER

#include <stdint.h>

/*Define the particle types*/
#define BARYON_TYPE 0
#define DM_TYPE 1
#define DISK_TYPE 2
#define NEUTRINO_TYPE 2
#define BULGE_TYPE 3
#define STARS_TYPE 4
#define BNDRY_TYPE 5
#define N_TYPE 6

  /** The Gadget simulation file header.
   */
 typedef struct {
    /** Number of particles of each type in this file*/
    uint32_t npart[N_TYPE];
    /** Mass of each particle type in this file. If zero, 
     * particle mass stored in snapshot.*/
    double   mass[N_TYPE];
    /** Time of snapshot*/
    double   time;
    /** Redshift of snapshot*/
    double   redshift;
    /** Boolean to test the presence of star formation*/
    int32_t  flag_sfr;
    /** Boolean to test the presence of feedback*/
    int32_t  flag_feedback;
    /** First 32-bits of total number of particles in the simulation*/
    uint32_t  npartTotal[N_TYPE];
    /** Boolean to test the presence of cooling */
    int32_t  flag_cooling;
    /** Number of files expected in this snapshot*/
    int32_t  num_files;
    /** Box size of the simulation*/
    double   BoxSize;
    /** Omega_Matter. Note this is Omega_DM + Omega_Baryons*/
    double   Omega0;
    /** Dark energy density*/
    double   OmegaLambda;
    /** Hubble parameter, in units where it is around 70. */
    double   HubbleParam; 
    /** Boolean to test whether stars have an age*/
    int32_t  flag_stellarage;
    /** Boolean to test the presence of metals*/
    int32_t  flag_metals;
    /** Long word of the total number of particles in the simulation. 
     * At least one version of N-GenICs sets this to something entirely different. */
    uint32_t  NallHW[N_TYPE];
    int32_t flag_entropy_instead_u;	/*!< flags that IC-file contains entropy instead of u */
    int32_t flag_doubleprecision;	/*!< flags that snapshot contains double-precision instead of single precision */

    int32_t flag_ic_info;             /*!< flag to inform whether IC files are generated with ordinary Zeldovich approximation,
                                       or whether they contain 2nd order lagrangian perturbation theory initial conditions.
                                       For snapshots files, the value informs whether the simulation was evolved from
                                       Zeldovich or 2lpt ICs. Encoding is as follows:
                                          FLAG_ZELDOVICH_ICS     (1)   - IC file based on Zeldovich
                                          FLAG_SECOND_ORDER_ICS  (2)   - Special IC-file containing 2lpt masses
                                          FLAG_EVOLVED_ZELDOVICH (3)   - snapshot evolved from Zeldovich ICs
                                          FLAG_EVOLVED_2LPT      (4)   - snapshot evolved from 2lpt ICs
                                          FLAG_NORMALICS_2LPT    (5)   - standard gadget file format with 2lpt ICs
                                       All other values, including 0 are interpreted as "don't know" for backwards compatability.
                                   */
    float lpt_scalingfactor;      /*!< scaling factor for 2lpt initial conditions */
      /** Fills header to 256 Bytes */
    char     fill[256- N_TYPE*sizeof(uint32_t)- (6+N_TYPE)*sizeof(double)- (9+2*N_TYPE)*sizeof(int32_t)-sizeof(float)];  
  } gadget_header;
 
#endif
