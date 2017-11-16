#ifndef MBMORE_SRC_ENRICH_FUNCTIONS_H_
#define MBMORE_SRC_ENRICH_FUNCTIONS_H_

#include <string>
#include <vector>

#include "cyclus.h"

namespace mbmore {

// LAPACK solver for system of linear equations
extern "C" {
void dgesv_(int *n, int *nrhs, double *a, int *lda, int *ipivot, double *b,
            int *ldb, int *info);
}

//group all the characteristic of a centrifuges
struct centrifuge_config {
  double v_a = 485;
  double height = 0.5;
  double diameter = 0.15;
  double feed = 15. / 1000. / 1000.;
  double temp = 320;
  double eff = 1.0;
  double M = 0.352;
  double dM = 0.003;
  double x = 1000;
  double flow_internal = 2.0;
};

// group all the characteristic of a stage
struct stg_config {
  double cut;
  double DU;
  double alpha;
  double beta;
  double flow;

  double n_machines;

  double feed_assay;
  double product_assay;
  double tail_assay;
};


//group charateristic of a full cascade
struct cascade_config {
  centrifuge_config cent_config;
  int stripping_stgs = 0;
  int enrich_stgs = 0;
  double feed_flow;
  std::map< int, stg_config> stgs_config;
};




// Organizes bids by enrichment level of requested material
bool SortBids(cyclus::Bid<cyclus::Material> *i,
              cyclus::Bid<cyclus::Material> *j);

// Calculates the ideal separation energy for a single machine
// as defined by the Raetz equation
// (referenced in Glaser, Science and Global Security 2009)
double CalcDelU(double cut, centrifuge_config centri_config);

// Calculates the exponent for the energy distribution using ideal gas law
// (component of multiple other equations)
double CalcCTherm(double v_a, double temp, double dM);

// Calculates the V(N_x) term for enrichment eqns where N_x is the assay
// of isotope x
double CalcV(double assay);

// Calculates the separations factor given the ideal separation energy of a
// single machine (del_U has units of moles/sec)
// Avery p 18
double AlphaBySwu(double del_U, double feed, double cut, double M);

// From Beta def in Glaser + Alpha def
double BetaByAlphaAndCut(double alpha, double feed_assay, double cut);
double CutByAalphaBeta(double alpha, double beta, double feed_assay);
// Calculates the assay of the product given the assay
// of the feed and the theoretical separation factor of the machine
// Glaser
double ProductAssayByAlpha(double alpha, double feed_assay);

// Calculates the assay of the waste given the assay
// of the feed and the theoretical separation factor of the machine
// Glaser
double TailAssayByBeta(double beta, double feed_assay);

// Calculates the number of stages needed in a cascade given the separation
// potential of a single centrifuge and the material assays
stg_config BuildIdealStg(double feed_assay, centrifuge_config cent_config,
                         double du = -1, double alpha = -1, double precision = 1e-16);

cascade_config FindNumberIdealStages(double feed_assay, double product_assay,
                                     double waste_assay,
                                     centrifuge_config cent_config,
                                     double precision = 1e-16); 

// Calculates the product assay after N enriching stages
double ProductAssayFromNStages(double alpha, double beta, double feed_assay,
                               double enrich_stages);

// Calculates the assay of the waste after N stripping stages
double TailAssayFromNStages(double alpha, double beta, double feed_assay,
                             double strip_stages);

// Number of machines in a stage (either enrich or strip)
// given the feed flow (stage_feed)
// flows do not have required units so long as they are consistent
// Feed flow of a single machine (in Avery denoted with L)
// Avery p. 62
double MachinesPerStage(double alpha, double del_U, double stage_feed);

// Flow of Waste (in same units and feed flow) in each stripping stage
// F_stage = incoming flow (in Avery denoted with L_r)
// Avery p. 60
double ProductPerEnrStage(double alpha, double feed_assay, double product_assay,
                          double stage_feed);

// Flow of Waste (in same units and feed flow) in each stripping stage
// F_stage = incoming flow (in Avery denoted with L_r)
// Avery p. 60
double WastePerStripStage(double alpha, double feed_assay, double waste_assay,
                          double stage_feed);

// Separation potential of the cascade
// ???
double DeltaUCascade(double product_assay, double waste_assay, double feed_flow,
                     double Pc);

// Number of machines in the cascade given the target feed rate and target
// assays and flow rates
// Avery 62
double MachinesPerCascade(double del_U_machine, double product_assay,
                          double waste_assay, double feed_flow,
                          double product_flow);

// Effective separation potential of a single machine when cascade is not
// being used in optimal configuration, as defined by the non-optimal
// assays and flow rates of the cascade
// ????
double DelUByCascade(double product_assay, double waste_assay,
                           double product_flow, double waste_flow,
                           double feed_assay);

// Solves system of linear eqns to determine steady state flow rates
// in each stage of cascade
cascade_config CalcFeedFlows(cascade_config cascade);

// Determines the number of machines and product in each stage based
// on the steady-state flows defined for the cascade.
cascade_config CalcStageFeatures(cascade_config cascade);

// Determine total number of machines in the cascade from machines per stage
int FindTotalMachines(cascade_config cascade);

cascade_config DesignCascade(cascade_config cascade, double max_feed, int max_centrifuges);

cascade_config Compute_Assay(cascade_config cascade_config,
                                 double feed_assay, double precision);

double Diff_enrichment(cascade_config actual_enrichments,
                       cascade_config previous_enrichement);

cascade_config Update_enrichment(cascade_config cascade,
                                     double feed_assay);

double get_cut_for_ideal_stg(centrifuge_config cent_config, double feed_assay,
                             double precision = 1e-16);
}  // namespace mbmore

#endif  //  MBMORE_SRC_ENRICH_FUNCTIONS_H_
