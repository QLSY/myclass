#include"anaspec_pppc.h"
#include"mydebug.h"
#include<iostream>
#include<sstream>
#include<fstream>
#include<cmath>


using std::istringstream;
using std::ifstream;
using std::log10;
using std::vector;
using std::cout;
using std::endl;

pppc::pppc_branch branch_map[] = {
  pppc::ele, pppc::mu, pppc::tau,
  pppc::W_boson,
  pppc::quark, //The quark channel of PPPC denotes a light quark
  pppc::charm, pppc::bottom, pppc::top,
  pppc::four_e, pppc::four_mu, pppc::four_tau
};

const vector <vector <pppc::pppc_product> > product_map = { {pppc::positrons}, {pppc::antiprotons}, {pppc::gammas}, {pppc::neutrinos_e, pppc::neutrinos_mu, pppc::neutrinos_tau}, {pppc::antideuterons} };
const vector <vector <double> > product_norm = { {1}, {1}, {1}, {2.0/3, 2.0/3, 2.0/3}, {1} };

anaspec_pppc::anaspec_pppc(): anaspec(), loaded(product_choice_size, false) { }
anaspec_pppc::anaspec_pppc(anaspec::product_choice product_): anaspec(product_), loaded(product_choice_size, false) {
}
anaspec_pppc::anaspec_pppc(double *branch_, anaspec::product_choice product_): anaspec(branch_, product_), loaded(product_choice_size, false) {

}
anaspec_pppc::anaspec_pppc(const std::vector <double> &branch_, anaspec::product_choice product_): anaspec(branch_, product_), loaded(product_choice_size, false) {

}

double anaspec_pppc::ask(double E, double mdm, bool cumulate) {
  printDebugMsg("Routine", ">>ask: E, m_dm, cumulate = %f, %f, %d", E, mdm, cumulate);
  double result = 0;
  if (!loaded[product]) load(product);
  for(unsigned i = 0; i < product_map[product].size(); i++)
    result += pppc_ask(E, mdm, product_map[product][i], cumulate) * product_norm[product][i];
  printDebugMsg("Routine", "<<ask: %f", result);
  return result;
}
double anaspec_pppc::pppc_ask(double E, double mdm, pppc::pppc_product prod, bool cumulate) {
  printDebugMsg("Routine", ">>pppc_ask: E, m_dm, cumulate = %f, %f, %d, %d", E, mdm, prod, cumulate);
  double result = 0, x = E/mdm, realx;
  for (unsigned i = 0; i < branch_choice_size; i++)
    if(branch[i]) {
      if(cumulate && x > low_x) {
        realx = x > 1 ? 1 : x;
        result += branch[i] * cumutable[prod][branch_map[i]].lnask(realx, mdm);
      } else if (x > low_x && x <= 1)
        result += branch[i] * table[prod][branch_map[i]].lnask(x, mdm) / mdm;
    }
  printDebugMsg("Routine", "<<pppc_ask: %f", result);
  return result;
}

int anaspec_pppc::load(pppc::pppc_product prod) {
#ifdef DATDIR
  string filename = DATDIR;
#else
  string filename = ".";
#endif
  filename += "/AtProduction_" + pppc::product_file[prod] +  ".dat";
  printDebugMsg("Routine", ">>load: filename = %s", filename.c_str());
  cout << "loading PPPC file " << filename << endl;
  ifstream dats(filename.c_str());

  string line;
  Table2D tab[pppc::pppc_branch_size], cumtab[pppc::pppc_branch_size];
  vector <double> xaxis, mass;

  getline(dats, line);

  int mind = -1, xind = -1;
  double Nx[pppc::pppc_branch_size],
         mold = 0, mnew, xnew, val; // xold = 0
  while (getline(dats, line)) {
    istringstream iss(line);
    iss >> mnew; iss >> xnew;

    if (mnew != mold) {
      xind = -1;
      mind++;
      mass.push_back(mnew);
      for (unsigned i = 0; i < pppc::pppc_branch_size; i++) Nx[i] = 0;
    }
    xind++;
    if (mind <= 0) xaxis.push_back(pow(10, xnew));

    for (unsigned i_branch = 0; i_branch < pppc::pppc_branch_size; i_branch++) {
      iss >> val;
      double dndx = val / (xaxis[xind] * log(10));

      if (xind != 0) Nx[i_branch] += (xaxis[xind] - xaxis[xind - 1]) * dndx;

      tab[i_branch].insval(xaxis[xind], mass[mind], dndx);
      cumtab[i_branch].insval(xaxis[xind], mass[mind], Nx[i_branch]);
    }

    mold = mnew;// xold = xnew;
  }

    for (unsigned i_branch = 0; i_branch < pppc::pppc_branch_size; i_branch++) {
      table[prod][i_branch].tabling(tab[i_branch]);
      cumutable[prod][i_branch].tabling(cumtab[i_branch]);
    }

   low_x = (tab[0].xaxis.begin())->first / 2;

   return 0;
}

int anaspec_pppc::load(anaspec::product_choice prod) {
  if (loaded[prod]) return 0;
  loaded[prod] = true;
  for (unsigned i = 0; i < product_map[prod].size(); i++)
    load(product_map[prod][i]);
  return 1;
}
