#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <unistd.h>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <assert.h>

const gsl_rng* RNG = gsl_rng_alloc (gsl_rng_taus2);

using namespace std;

template <typename T> inline T sum(vector<T> list) { T sum=0; for (unsigned int i=0; i<list.size(); i++) sum += list[i]; return sum;}

const double EPSILON = 10e-15;

/*
#!/usr/bin/python
from random import random, shuffle, sample, choice, randint
from collections import OrderedDict
from sys import exit
from copy import deepcopy
from sys import argv, exit

if len(argv) != 3:
    print "\n\tUsage: ./yucatan_immunity_generator.py <expansion_factor> <last_year>" 
    print "\tNB: <last_year> is the last year of data to include when simulating immunity.\n"
    exit()
*/

const int NUM_SEROTYPES = 4;
const int MAX_CENSUS_AGE = 85;// used if a census age group has only a minimum value, e.g. '85+'

// Reported DF + DHF cases, 1997-2011 (inclusive)

const vector<int> CASES =                                 {4234, // 1979
     4672, 3377, 1412,  643, 5495,  193,   34,   15,  356,    2, // 1980-1989
        8,  352,   22,   29,  680,   69,  650, 5529,   36,   43, // 1990-1999
        0,  287,  946,   26,   57,  162,  627, 1861,  721, 3212, // 2000-2009
     2517, 6132, 5705};                                          // 2010-2012

const float FRACTION_SEROTYPED = 0.056; // based on Casos Históricos.xlsx from Hector
 
const vector< vector<double> > SEROTYPE_WT = { 
    {1.00,  0.00,  0.00,  0.00},   // 1979 // Fig. 1
    {1.00,  0.00,  0.00,  0.00},   // 1980 // Fig. 1
    {1.00,  0.00,  0.00,  0.00},   // 1981 // Fig. 1
    {1.00,  0.00,  0.00,  0.00},   // 1982 // Fig. 1
    {1.00,  0.00,  0.00,  0.00},   // 1983 // Fig. 1
    {0.50,  0.00,  0.00,  0.50},   // 1984 // Fig. 1
    {1.00,  0.00,  0.00,  0.00},   // 1985 // Fig. 1
    {0.00,  1.00,  0.00,  0.00},   // 1986 // Fig. 1
    {1.00,  0.00,  0.00,  0.00},   // 1987 // Fig. 1
    {1.00,  0.00,  0.00,  0.00},   // 1988 // Fig. 1
    {1.00,  0.00,  0.00,  0.00},   // 1989 // Fig. 1
    {1.00,  0.00,  0.00,  0.00},   // 1990 // Fig. 1
    {0.50,  0.50,  0.00,  0.00},   // 1991 // Fig. 1
    {1.00,  0.00,  0.00,  0.00},   // 1992 // Fig. 1
    {1.00,  0.00,  0.00,  0.00},   // 1993 // Fig. 1
    {0.33,  0.33,  0.00,  0.34},   // 1994 // Fig. 1
    {0.50,  0.50,  0.00,  0.00},   // 1995 // Fig. 1
    {0.25,  0.25,  0.25,  0.25},   // 1996 // Fig. 1
    {0.09,  0.00,  0.87,  0.04},   // 1997 // Fig. 4
    {0.09,  0.00,  0.87,  0.04},   // 1998 // extrapolated
    {0.09,  0.00,  0.87,  0.04},   // 1999 // extrapolated
    {0.09,  0.00,  0.87,  0.04},   // 2000 // extrapolated
    {0.00,  0.60,  0.40,  0.00},   // 2001 // Fig. 4
    {0.04,  0.96,  0.00,  0.00},   // 2002 // Fig. 4
    {0.04,  0.96,  0.00,  0.00},   // 2003 // extrapolated
    {0.04,  0.96,  0.00,  0.00},   // 2004 // extrapolated
    {0.11,  0.89,  0.00,  0.00},   // 2005 // Fig. 4
    {0.27,  0.55,  0.18,  0.00},   // 2006 // Fig. 4
    {0.90,  0.04,  0.04,  0.02},   // 2007 // Fig. 4
    {0.85,  0.15,  0.00,  0.00},   // 2008 // Fig. 4
    {0.46,  0.54,  0.00,  0.00},   // 2009 // Fig. 4
    {0.59,  0.41,  0.00,  0.00},   // 2010 // Fig. 4
    {0.32,  0.68,  0.00,  0.00},   // 2011 // Fig. 4
    {0.34,  0.66,  0.00,  0.00}};  // 2012 // Fig. 4


// Devroye's algorithm, as described by Kachitvichyanukul and Schmeiser (1988), 
// "Binomial random variate generation."  I've correct two mistakes in the publication:
// the original fails if p = 1, and the conditional for the while loop should be y <= n, 
// rather than y < n.  The latter precludes ever drawing a deviate of n for p < 1.

// Algorithm BG
// 1. Set y <-- 0, x <-- 0, c <-- ln(1 - p).
// 2. If c == 0, return x.
// 3. Generate u ~ U(0, 1).
// 4. y <-- y + floor(ln(u)/c) + 1.
// 5. If y < n, set x <-- x + 1, and goto 3.
// 6. Return x.
/*
int rand_binomial (int n, double p) {
    if ( p == 1.0 ) {
        return n;
    } else if ( p == 0.0 or n == 0 ) {
        return 0;
    }

    int y = 0; 
    int x = 0;
    double c = log( 1 - p ); // p can't be 0, but we've already checked that

    while ( y <= n ) {
        double u = gsl_rng_uniform(RNG);
        y += (int) (log(u)/c) + 1;
        if (y > n) {
            return x;
        }
        x += 1;
    }
    return -1; // bad input was provided
}*/


vector<int> bootstrap_cases(const double EF) {
    vector<int> bt_cases;
    for (int case_ct: CASES) bt_cases.push_back( gsl_ran_binomial(RNG, 1.0/EF, (int) case_ct*EF) );
    //for (int c: CASES) bt_cases.push_back( rand_binomial(c*EF, 1.0/EF) );
    return bt_cases;
}


vector< vector<double> > bootstrap_serotypes(const vector<int>& cases) {
    vector<vector<double> > bt_serotype_wt (SEROTYPE_WT.size(), vector<double>(NUM_SEROTYPES, 0));
   
    assert (cases.size() == SEROTYPE_WT.size());
    for (unsigned int i = 0; i < SEROTYPE_WT.size(); ++i) {
        const int num_tested = gsl_ran_binomial(RNG, FRACTION_SEROTYPED, cases[i]);
//cerr << 1979 + i << " " << cases[i] << " " << num_tested << endl;
        if (num_tested > 0) {
            double p[NUM_SEROTYPES];
            copy(SEROTYPE_WT[i].begin(), SEROTYPE_WT[i].end(), p);
            unsigned int case_cts[NUM_SEROTYPES] = {0,0,0,0};
            gsl_ran_multinomial(RNG, NUM_SEROTYPES, num_tested, p, case_cts);
            for (unsigned int s = 0; s < NUM_SEROTYPES; ++s) bt_serotype_wt[i][s] = (double) case_cts[s] / num_tested;
        } else {
            if (i > 0) {
                // if we determined that no cases were serotyped for this year, then assume the same serotype
                // distribution that was simulated from the previous year
                for (unsigned int s = 0; s < NUM_SEROTYPES; ++s) bt_serotype_wt[i][s] = bt_serotype_wt[i-1][s];
            } else {
                // if this is year one, so no previous data existed, just revert to the non-bootstraped data
                for (unsigned int s = 0; s < NUM_SEROTYPES; ++s) bt_serotype_wt[i][s] = SEROTYPE_WT[i][s];
            }
        }
    }
    return bt_serotype_wt;
}


long seedgen(void)  {
    long s, seed, pid, seconds;

    pid = getpid();
    s = time ( &seconds ); /* get CPU seconds since 01/01/1970 */

    seed = abs(((s*181)*((pid-83)*359))%104729); 
    return seed;
}


int sample_serotype(vector<double> dist) {
    double last = 0;
    double rand = gsl_rng_uniform(RNG);
    for (unsigned int i = 0; i < dist.size(); i++ ) {
        double current = last + dist[i];
        if ( rand <= current ) {
            return i;
        } else {
            last = current;
        }
    }
    if (fabs(last - 1) > EPSILON) {
        cerr << "rand_nonuniform_int() expects a normed distribution.  "
            << "Your probabilities sum to " << setprecision(15) << last << endl;
        exit(1);
    }
    return -1;
}


struct AgeTally {
    AgeTally(string a, int t) { age=a; tally=t; }
    string age;
    int tally;
};


void split(const string& s, char c, vector<string>& v) {
    string::size_type i = 0;
    string::size_type j = s.find(c);

    while (j != string::npos) {
        v.push_back(s.substr(i, j-i));
        i = ++j;
        j = s.find(c, j);
    }
    if (j == string::npos) v.push_back(s.substr(i, s.length( )));
}


map<int,vector<AgeTally> > import_census_data(string filename) {
/*
year 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85+
1970 26222 25943 25663 25384 25105 24825 24546 24267 23433 22599 21765 20931 20098 19254 18411 17568 16725 15882 15239 14597 13955 13312 12670 12185 11700 11215 10730 10245 9827 9410 8993 8575 8158 8105 8051 7998 7945 7892 7529 7165 6802 6438 6075 5889 5702 5516 5330 5144 4864 4584 4305 4025 3746 3632 3519 3406 3293 3179 3120 3061 3002 2943 2884 2749 2613 2478 2343 2208 2073 1938 1804 1669 1534 1386 1238 1090 942 794 749 704 659 614 569 523 478 2623
1971 26661 26406 26150 25895 25639 25384 25128 24873 24070 23267 22464 21662 20859 20030 19201 18372 17542 16713 16041 15369 14697 14025 13353 12844 12335 11826 11317 10808 10377 9946 9514 9083 8652 8576 8499 8423 8346 8270 7888 7507 7125 6743 6361 6167 5972 5777 5583 5388 5102 4817 4531 4245 3960 3836 3712 3588 3464 3340 3274 3209 3143 3078 3012 2869 2727 2584 2441 2298 2158 2018 1878 1738 1598 1449 1300 1151 1002 852 803 754 705 656 607 558 509 2804
*/
    map<int, vector<AgeTally> > census;
    ifstream censusfile(filename);
    // We need to add a check in case the file doesn't exist
    string line; 
    vector<string> ages;
    while(getline(censusfile,line)) {
        vector<string> fields;
        split(line, ' ', fields);
        int year;
        if (fields[0] == "year") {
            for (unsigned k = 1; k < fields.size(); k++) ages.push_back(fields[k]);
            continue;
        } else if (fields.size()) {          
            year = stoi(fields[0]);
            for(unsigned int i=1; i < fields.size(); i++ ) {  
                census[year].push_back( AgeTally(ages[i-1],stoi(fields[i])) );
                //cerr << year << " " << ages[i-1] << " " << fields[i] << endl;
            }
        }
    }
    return census;
}


bool recently_infected(vector<int> states) {
    // "recently infected" means in the last one or two years
    for (unsigned int i = 0; i < states.size(); i++) {
        const int state = states[i];
        if (state == 1 or state == 2) {
            return true;
        }
    }
    return false;
}


int age_str_to_int(string age_str) {
    int age;
    try {
        age = stoi(age_str);
    } catch (const std::invalid_argument& ia ) {
        age = MAX_CENSUS_AGE;
    }
    return age;
}


vector< vector< vector<int> > > initialize_full_population(const map<int,vector<AgeTally> >& census, int first_year) {
    vector< vector< vector<int> > > full_pop(MAX_CENSUS_AGE + 1, vector< vector<int> >());
    //full_pop = [[] for i in range(MAX_CENSUS_AGE + 1)]

    for (unsigned int i = 0; i < census.at(first_year).size(); ++i) {
        string age_str = census.at(first_year)[i].age;
        int age = age_str_to_int(age_str);
        for (int k = 0; k < census.at(first_year)[i].tally; ++k) {
            full_pop[age].push_back(vector<int>(NUM_SEROTYPES,0));
        }
    }
    return full_pop;
}

void age_immunity(vector< vector< vector<int> > >& pop) {
    for (unsigned int age = 0; age < pop.size(); ++age) { // each age group
        for (unsigned int i = 0; i < pop[age].size(); ++i) { // each individual
            for (unsigned int s = 0; s < pop[age][i].size(); ++s) { // each serotype
                if (pop[age][i][s] >= 1) pop[age][i][s]++;
            }
        }
    }
}


int extract_tally(const vector<AgeTally> census_year, string age_str) {
    for (unsigned int i = 0; i < census_year.size(); ++i) { 
        if (census_year[i].age == age_str) return census_year[i].tally;
    }
    return 0;
}


void age_full_population(const map<int,vector<AgeTally> >& census, vector< vector< vector<int> > >& full_pop, int new_year) {
/*
"Age" the population by sampling from younger age classes as necessary to make up
the census numbers for the new year, e.g. if there are ten 30-year-olds in 1980,
and we need eight 31-year-olds in 1981, we will randomly select 8 of the 10.  If
we need MORE than the number available, we will sample with replacement to get the
required number.

Age 0 individuals are special because they don't have a younger age class to
inherit from.  MAX_CENSUS_AGE individuals are likely special because they may be
in a binned age class with no specified upper bound.  This is tricky because 85+
individuals in 1980 who survive to 1981 are still 85+, but some of the age 84
individuals from 1980 have joined their ranks.  Because of this, for the eldest
age class we sample from the two eldest groups.  This does not account for
differing mortality rates among the group, but since it's a relatively small
fraction of the population that is likely to have relatively homogenous immunity
(due to their long exposure to epidemics), this assumption is probably not
consequential.

Note: This will fail if there is a year with 0 individuals in an age class other
than the last.
*/
    age_immunity(full_pop);
    vector<string> age_keys;
    for (unsigned int a = 0; a < census.at(new_year).size(); ++a) age_keys.push_back(census.at(new_year)[a].age);
    reverse(age_keys.begin(),age_keys.end());

    
    for (unsigned int i = 0; i < age_keys.size(); ++i) { // going from oldest to youngest
        string age_str = age_keys[i];
        int age = age_str_to_int(age_str);
        int tally = extract_tally(census.at(new_year), age_str);
        if (age == 0) {
            // nothing to inherit for infants
            full_pop[age] = vector< vector<int> > (tally, vector<int> (NUM_SEROTYPES,0));
            continue;
        }
        vector< vector<int> > sampling_pop = full_pop[age - 1];
        if (age == MAX_CENSUS_AGE) { // We are assuming here that the last age class is a multi-year bin, eg. 85+
            sampling_pop.insert(sampling_pop.end(), full_pop[MAX_CENSUS_AGE].begin(), full_pop[MAX_CENSUS_AGE].end());
        } 
        vector< vector<int> > new_pop;
        for (int k = 0; k < tally; ++k) {
            int r = gsl_rng_uniform_int(RNG, sampling_pop.size());
            new_pop.push_back(sampling_pop[r]); 
        }
        //print "year, age, sample size, census size: ", new_year, age, len(sampling_pop), census[new_year][age_str]
        full_pop[age] = new_pop;
    }
} 


vector<int> tally_pop_by_age(vector<vector<vector<int > > > full_pop) {
    vector<int> tallies(full_pop.size(), 0);
    for (unsigned int age = 0; age < full_pop.size(); ++age) {
        tallies[age] = full_pop[age].size();
    }
    return tallies;
}


void output_immunity_file(string filename, const vector<vector<vector<int> > >& full_pop) {
    bool header = true;
    ofstream fo(filename);
    
    fo << "pid age imm1 imm2 imm3 imm4\n";
    cout << "Building immunity file . . .\n";

    int counter = 0;
    // read in population data
    ifstream  infile("../../pop-yucatan/population-yucatan.txt");
    string line;
    while(getline(infile,line)) {
        if(header) { header=false; continue; }
        if(counter % 100000 == 0 ) { cerr << counter << endl; }
        counter++;
        vector<string> p;
        split(line, ' ', p);
        string pid = p[0];
        int age = stoi(p[2]);
        
        age = age <= MAX_CENSUS_AGE ? age : MAX_CENSUS_AGE;
        int r = gsl_rng_uniform_int(RNG, full_pop[age].size());
        vector<int> states = full_pop[age][r];
        fo << pid << " " << age << " ";
        for (int s: states) fo << s << " ";
        fo << endl;
    }
    fo.close();
}


int main(int argc, char** argv) {
    const bool bootstrap_data = true;
    const bool write_immunity_file = false;

    if (argc != 2) {
        cerr << "\n\tUsage: ./immgen <last_year_to_simulate>\n\n";
        //cerr << "\n\tUsage: ./immgen <expansion_factor> <last_year_to_simulate>\n\n";
        exit(100);
    }

    gsl_rng_set(RNG, seedgen());
    //const float EXPANSION_FACTOR = atof(argv[1]);
    const int FIRST_YEAR = 1979;
    const int LAST_YEAR = atoi(argv[1]);
    const int MAX_EF = 50;
    const int MIN_EF = 25;

    vector<int> YEARS(LAST_YEAR - FIRST_YEAR + 1);
    iota(YEARS.begin(), YEARS.end(), FIRST_YEAR);

    const map<int, vector<AgeTally> > census = import_census_data("interpolated_ages-yucatan.out");

    const int REPS = 10000;

    for (int rep = 0; rep < REPS; ++rep) {

        const float EXPANSION_FACTOR = gsl_rng_uniform(RNG)*(MAX_EF - MIN_EF) + MIN_EF;
        vector<int> cases;
        vector<vector<double>> serotype_wt;
        vector<vector<vector<int> > > full_pop = initialize_full_population(census, FIRST_YEAR);
        
        if (bootstrap_data) {
            cases = bootstrap_cases(EXPANSION_FACTOR);
            serotype_wt = bootstrap_serotypes(cases);
        } else {
            cases = CASES;
            serotype_wt = SEROTYPE_WT;
        }

        /*for (unsigned int y = 0; y < SEROTYPE_WT.size(); ++y) {
            cout << setprecision(4);
            for ( double w: SEROTYPE_WT[y] ) cout << w << " ";
            cout << endl;
            for ( double w: serotype_wt[y] ) cout << w << " ";
            cout << endl << endl;
        }*/

        int year = 0;
        int kids_in_1987 = 0; // in the 8-14 range
        float seropositive_kids_in_1987 = 0;  // looking to see ~60% of the population

        for (unsigned int ya = YEARS.size(); ya > 0; --ya) { 
            if (year > 0) age_full_population(census, full_pop, YEARS[year]);
            int num_of_infections = (int) (cases[year] * EXPANSION_FACTOR + 0.5);

            vector<int> pop_size_by_age = tally_pop_by_age(full_pop);
            int pop_size = accumulate(pop_size_by_age.begin(), pop_size_by_age.end(), 0);
            cout << "year, years ago, pop, cases, infections, expansion factor\n";
            cout << YEARS[year] << " " <<  ya << " " <<  pop_size << " " <<  cases[year] << " " <<  num_of_infections << " " <<  EXPANSION_FACTOR << endl;
            cout << "expected serotype distribution: "; for (double wt: serotype_wt[year]) cout << wt << " "; cout << endl;
            vector<int> serotype_tally(NUM_SEROTYPES, 0);
            for (int i = 0; i < num_of_infections; ++i) {
                int s = sample_serotype(serotype_wt[year]);
                bool infection_occurred = false;
                while (not infection_occurred) { 
                    int test = gsl_rng_uniform_int(RNG, pop_size);
                    int age = 0;
                    while (test >= pop_size_by_age[age]) {
                        test -= pop_size_by_age[age];
                        age++;
                    }
                    if (full_pop[age][test][s] == 0 and not recently_infected(full_pop[age][test])) {
                        // infect test person
                        full_pop[age][test][s] = 1;
                        serotype_tally[s]++;
                        infection_occurred = true;
                    }
                }
            }
            cout << "actual serotype counts: "; for (int t: serotype_tally) cout << t << " "; cout << endl;
            if (num_of_infections > 0) {
                const double total = (double) sum(serotype_tally);
                cout << "actual serotype frequency: "; for(int c: serotype_tally)  cout << c/total << " "; cout << endl;
            } else {
                cout << "actual serotype frequency: NA NA NA NA\n";
            }
            
            if (YEARS[year] == 1987) {
                for (int age = 8; age < 15; ++age) {
                    for (auto kid: full_pop[age]) {
                    //for (vector<int> kid: full_pop[age]) {
                        kids_in_1987 += 1.0;
                        auto seropos = sum(kid);
                        if (seropos > 0) {
                            seropositive_kids_in_1987 += 1.0;
                        }
                     }
                }
                cout << "Number of kids 8-14 in 1987: " << kids_in_1987 << endl;
                cout << "Number of seropositive kids 8-14 in 1987: " << seropositive_kids_in_1987 << endl;
                cout << "Fraction seropositive: " << seropositive_kids_in_1987/kids_in_1987 << endl;
                cout << "Empirical fraction seropositive: ~0.6" << endl;
                cout << "Estimated expansion factor correction: " << 0.6/(seropositive_kids_in_1987/kids_in_1987) << endl;
                cerr << EXPANSION_FACTOR << " " << seropositive_kids_in_1987/kids_in_1987 << endl;
            }
            year++;
            cout << endl;
        }
        cout << "\t\t\tDone.\n";

        //string immunity_filename = "/work/01856/thladish/initial_immunity/" + to_string(EXPANSION_FACTOR) + ".txt";
        if (write_immunity_file) {
            string immunity_filename = to_string(EXPANSION_FACTOR) + ".txt";
            output_immunity_file(immunity_filename, full_pop);
        }

        cout << "\t\t\tDone.\n";
    }

    return 0;
}


