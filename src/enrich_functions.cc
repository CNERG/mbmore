#define _USE_MATH_DEFINES

#include "enrich_functions.h"
#include <ctime> // to make truly random
#include <cstdlib>
#include <iostream>
#include <cmath>
#include <iterator>

namespace mbmore {

  double D_rho = 2.2e-5; // kg/m/s
  double gas_const = 8.314; // J/K/mol
  double M_238 = 0.238; //kg/mol


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO:
// annotate assumed units, Glaser paper reference

  double CalcDelU(double v_a, double height, double diameter,  double feed,
		  double temp, double cut, double eff, double M_mol, double dM,
		  double x, double flow_internal) {

    // Inputs that are effectively constants:
    // flow_internal = 2-4, x = pressure ratio, M = 0.352 kg/mol of UF6,
    // dM = 0.003 kg/mol diff between 235 and 238

    double a = diameter / 2.0;  // outer radius

    // withdrawl radius for heavy isotope
    // Glaser 2009 says operationally it ranges from 0.96-0.99
    double r_2 = 0.99 * a; 

    double r_12 = std::sqrt(1.0 - (2.0 * gas_const * temp*(log(x)) / M_mol /
				   (pow(v_a, 2)))); 
    double r_1 = r_2 * r_12; // withdrawl radius for ligher isotope

    // Glaser eqn 12
    // Vertical location of feed
    double Z_p = height * (1.0 - cut) * (1.0 + flow_internal) /
      (1.0 - cut + flow_internal);

    //Glaser eqn 3
    // To make consistent with Glaser results:
    //    double C1 = (2.0 * M_PI * (D_rho*M_238/M_mol) / (log(r_2 / r_1)));
    double C1 = (2.0 * M_PI * D_rho / (log(r_2 / r_1)));
    double A_p = C1 *(1.0 / feed) *
      (cut / ((1.0 + flow_internal) * (1.0 - cut + flow_internal)));
    double A_w = C1 * (1.0 / feed) *
      ((1.0 - cut)/(flow_internal * (1.0 - cut + flow_internal)));
     
    double C_therm = CalcCTherm(v_a, temp, dM);

    // defining terms in the Ratz equation
    double r12_sq = pow(r_12, 2);
    double C_scale = (pow((r_2 / a), 4)) * (pow((1 - r12_sq), 2));
    double bracket1 = (1 + flow_internal) / cut;
    double exp1 = exp(-1.0 * A_p * Z_p);
    double bracket2 = flow_internal/(1 - cut);
    double exp2 = exp(-1.0 * A_w * (height - Z_p));

    // Glaser eqn 10 (Ratz Equation)
    double major_term = 0.5 * cut * (1.0 - cut) * (pow(C_therm, 2)) * C_scale *
      pow(((bracket1 * (1 - exp1)) + (bracket2 * (1 - exp2))), 2); // kg/s    
    double del_U = feed * major_term * eff; // kg/s
    
    return del_U;
  }

  double CalcCTherm(double v_a, double temp, double dM) {
    double c_therm = (dM * (pow(v_a, 2)))/(2.0 * gas_const * temp);
    return c_therm;
  }

  double CalcV(double assay){
    return (2.0 * assay - 1.0) * log(assay / (1.0 - assay));
  }

  double AlphaBySwu(double del_U, double feed, double cut, double M){
    double alpha = 1 + std::sqrt((2 * (del_U / M) * (1 - cut) / (cut * feed)));
    return alpha;
  }

  // per machine
  double ProductAssayByAlpha(double alpha, double feed_assay){
    // Possibly incorrect is commented out ? 
    //    double ratio = (1.0 - feed_assay) / (alpha * feed_assay);
    //    return 1.0 / (ratio + 1.0);
    double ratio = alpha * feed_assay/(1.0 - feed_assay);
    return ratio / (1 + ratio);
  }

  double WasteAssayByAlpha(double alpha, double feed_assay){
    double A = (feed_assay / (1 - feed_assay)) / alpha;
    return A / (1 + A);
  }

  // This equation can only be used in the limit where the separation factor
  // (alpha) is very close to one, which is not true for modern gas centrifuges
  // DO NOT USE THIS EQUATION!!!
  // Avery p.59
  /*
  std::pair<double, double> StagesPerCascade(double alpha, double feed_assay,
					     double product_assay,
					     double waste_assay){
    using std::pair;

    double epsilon = alpha - 1.0;
    double enrich_inner = (product_assay / (1.0 - product_assay)) *
      ((1.0 - feed_assay) / feed_assay);
    double strip_inner =  (feed_assay / (1.0 - feed_assay)) *
      ((1.0 - waste_assay) / waste_assay);

    double enrich_stages = (1.0 / epsilon) * log(enrich_inner);
    double strip_stages = (1.0 / epsilon) * log(strip_inner);
    std::pair<double, double> stages = std::make_pair(enrich_stages,
						      strip_stages);
    return stages;

  }
  */

 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // Determine number of stages required to reach ideal cascade product assay
// (requires integer number of stages, so output may exceed target assay)

std::pair<int, int> FindNStages(double alpha, double feed_assay,
					     double product_assay,
					     double waste_assay){
    using std::pair;

    int ideal_enrich_stage = 0;
    int ideal_strip_stage = 0;
    double stage_feed_assay = feed_assay;
    double stage_product_assay = feed_assay;
    double stage_waste_assay = feed_assay;  //start w/waste of 1st enrich stage

    // Calculate number of enriching stages
    while (stage_product_assay < product_assay){
      stage_product_assay = ProductAssayByAlpha(alpha, stage_feed_assay);
      if (ideal_enrich_stage == 0){
	stage_waste_assay = WasteAssayByAlpha(alpha, stage_feed_assay);
      }
      ideal_enrich_stage +=1;
      stage_feed_assay = stage_product_assay;
    }
    // Calculate number of stripping stages
    stage_feed_assay = stage_waste_assay;
    while (stage_waste_assay > waste_assay){
      stage_waste_assay = WasteAssayByAlpha(alpha, stage_feed_assay);
      ideal_strip_stage += 1;
      stage_feed_assay = stage_waste_assay;
    }
    
    std::pair<int, int> stages = std::make_pair(ideal_enrich_stage,
						ideal_strip_stage);
    return stages;
    
  }
  
  double ProductAssayFromNStages(double alpha, double feed_assay,
			    double enrich_stages){
    double A = (feed_assay / (1 - feed_assay)) *
      exp(enrich_stages * (alpha - 1.0));
    double product_assay = A / (1 + A);
    return product_assay;
  }
  
  double WasteAssayFromNStages(double alpha, double feed_assay,
			       double strip_stages){
    return 1/(1 + ((1 - feed_assay) / feed_assay) *
	      exp(strip_stages * (alpha - 1.0)));
  }

  double MachinesPerStage(double alpha, double del_U, double stage_feed){
    return stage_feed / (2.0 * del_U / (pow((alpha - 1.0), 2)));
  }

  double ProductPerEnrStage(double alpha, double feed_assay,
			    double product_assay, double stage_feed){
    return stage_feed * (alpha - 1.0) * feed_assay * (1 - feed_assay) /
      (2 * (product_assay - feed_assay));
  }
  
  // 14-Feb-2017
  // THIS EQN PRODUCES THE WRONG RESULT FOR SOME REASON.
  // DONT KNOW WHAT THE PROBLEM IS THOUGH
  /*
  double WastePerStripStage(double alpha, double feed_assay, double waste_assay,
			    double stage_feed){
    return stage_feed * (alpha - 1.0) * feed_assay * (1 - feed_assay) /
      (2 * (feed_assay - waste_assay));
  }
  */
  
  double DeltaUCascade(double product_assay, double waste_assay,
		       double feed_flow, double product_flow){
    double Vpc = CalcV(product_assay);
    double Vwc = CalcV(waste_assay);
    return product_flow * Vpc + (feed_flow - product_flow) * Vwc;
  }
  
  double MachinesPerCascade(double del_U_machine, double product_assay,
			    double waste_assay, double feed_flow,
			    double product_flow){
    double U_cascade = DeltaUCascade(product_assay, waste_assay, feed_flow,
				     product_flow);
    return U_cascade / del_U_machine;
  }

  double DelUByCascadeConfig(double product_assay, double waste_assay,
			     double product_flow, double waste_flow,
			     double feed_assay){
    double U_cascade = DeltaUCascade(product_assay, waste_assay, product_flow,
				     waste_flow);
    return U_cascade / feed_assay;
  }


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Calculate steady-state flow rates into each cascade stage
// Linear system of equations in form AX = B, where A is nxn square matrix
// of linear equations for the flow rates of each stage and B are the external
// feeds for the stage. External feed is zero for all stages accept cascade
// feed stage (F_0) stages start with last strip stage [-2, -1, 0, 1, 2]
//
//double** CalcFeedFlows(std::pair<double, double> n_st,
//  void CalcFeedFlows(std::pair<double, double> n_st,
  std::vector<double> CalcFeedFlows(std::pair<double, double> n_st,
				    double cascade_feed, double cut){
    
    // This is the Max # of stages in cascade. It cannot be passed in due to
    // how memory is allocated and so must be hardcoded. It's been chosen
    // to be much larger than it should ever need to be
    int max_stages = 100;
    
    int n_enrich = n_st.first;
    int n_strip = n_st.second;
    int n_stages = n_st.first + n_st.second;
    //    std::cout << "enrich # " << n_enrich << " strip # " << n_strip << std::endl; 
    
    //LAPACK takes the external flow feeds as B, and then returns a modified
    // version of the same array now representing the solution flow rates.
 
    // Build Array with pointers
    //*    double** flow_eqns = new double*[n_stages];
    double flow_eqns[max_stages][max_stages];
    //    double** flows = new double*[n_stages];
    double flows[1][max_stages];
    //*    double** flows = new double*[1];

    //    for (int i = 0; i < n_stages; ++i){
    //      flow_eqns[i] =  double[n_stages];
    //      //  flows[i] = new double[1];
    //    }
 
    
    /*
    // Build array a vector
    std::vector<std::vector<double>> flow_eqns;
    std::vector<double> extern_feed;
    //    std::vector<double> flow_solns;

    std::cout << "Matrix: " << std::endl;
    for (int i = 0; i < n_stages; i++){
      extern_feed.push_back(0);
      //      flow_solns.push_back(0);
      std::vector<double> matrix_builder;
      for (int j = 0;  j < n_stages; j++){
	matrix_builder.push_back(0);
      }
      for (auto p = matrix_builder.begin(); p != matrix_builder.end(); ++p){
	std::cout << *p << ' ';
      }
      std::cout << std::endl;
      flow_eqns.push_back(matrix_builder);
    }
    */
     
    // build matrix of equations in this pattern
    // [[ -1, 1-cut,    0,     0,      0]       [[0]
    //  [cut,    -1, 1-cut,    0,      0]        [0]       
    //  [  0,   cut,    -1, 1-cut,     0]  * X = [-1*cascade_feed]
    //  [  0,     0,   cut,    -1, 1-cut]        [0]
    //  [  0,     0,     0,   cut,    -1]]       [0]]
    //

    
    for (int row_idx = 0; row_idx < max_stages; row_idx++){
      // fill the array with zeros, then update individual elements as nonzero
      flows[0][row_idx] = 0;
      for (int fill_idx = 0; fill_idx < max_stages; fill_idx++){
	flow_eqns[fill_idx][row_idx] = 0;
      }
      // Required do to the artificial 'Max Stages' defn. Only calculate
      // non-zero matrix elements where stages really exist.
      if (row_idx < n_stages){
	int i = row_idx - n_strip;
	int col_idx = n_strip + i;
	flow_eqns[col_idx][row_idx] = -1;
	if (col_idx != 0){
	  flow_eqns[col_idx - 1][row_idx] = cut ;
	}
	if (col_idx != n_stages - 1){
	  flow_eqns[col_idx + 1][row_idx] = (1-cut);
	}
	// Add the external feed for the cascade
	if (i == 0){
	  flows[0][row_idx] = -1*cascade_feed;
	}
	
	std::cout << "Row " << row_idx << std::endl;
	std::cout << "  " << flows[0][row_idx] << "  " ;
	for (int j = 0; j < n_stages; j++){
	  //	std::cout << "  " << flow_eqns[j][row_idx] << "  " ;
	}
	std::cout << std::endl;
      }
    }
    
    //      return np.linalg.solve(eqn_array, eqn_answers)
    
    /*
      std::cout << "Feed vector" << std::endl;
      for (auto p = extern_feed.begin(); p != extern_feed.end(); ++p){
      std::cout << *p << ' ' << std::endl;
      }
    */
    
    
    //double a[MAX][MAX];  -- flow_eqns
    //    double b[1][MAX]; -- RHS (extern_feed), and THEN the result
    //int n=5;  -- n_stages
    
    int nrhs = 1; // 1 column solution
    int lda = max_stages;  // must be >= MAX(1,N)
    int ldb = max_stages;       // must be >= MAX(1,N)
    int ipiv[max_stages];
    int info;
    
    // Solve the linear system
    dgesv_(&n_stages, &nrhs, &flow_eqns[0][0], 
	   &lda, ipiv, &flows[0][0], &ldb, &info);
    
    
    // Check for success
    if(info == 0){
      // Write the answer
      std::cout << "The answer is\n";
      for(int i = 0; i < n_stages; i++){
        std::cout << "b[" << i << "]\t" << flows[0][i] << "\n";
      }
    }
    else {
      // Write an error message
      std::cerr << "dgesv returned error " << info << "\n";
    }

    std::vector<double> final_flows;
    for(int i = 0; i < n_stages; i++){
      final_flows.push_back(flows[0][i]);
    }					      
    return final_flows;
    
  }

  
} // namespace mbmore
