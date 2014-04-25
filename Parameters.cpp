#include "Parameters.h"
#include "Location.h"

void Parameters::readParameters(int argc, char *argv[]) {
        randomseed = 5489;
        nRunLength = 100;
        betaPM = 0.2;
        betaMP = 0.1;
        fMosquitoMove = 0.2;
        szMosquitoMoveModel = "weighted";
        fMosquitoTeleport = 0.01;
        fVEI = 0.0;
        fVEP = 0.0;
        fVESs.clear(); fVESs.resize(NUM_OF_SEROTYPES, 0.95);
        bVaccineLeaky = false;
        fPreVaccinateFraction = 0.0;
        nDefaultMosquitoCapacity = 20;                      // mosquitoes per location
        eMosquitoDistribution = CONSTANT;
        nSizeMosquitoMultipliers = 0;
        nSizeExternalIncubation = 0;
        bSecondaryTransmission = true;
        szPopulationFile = "population-64.txt";
        szImmunityFile = "";
        szNetworkFile = "locations-network-64.txt";
        szLocationFile = "locations-64.txt";
        szPeopleFile = "";
        szYearlyPeopleFile = "";
        szDailyFile = "";
        szSwapProbFile = "";
        nDaysImmune = 365;
        nSizeVaccinate = 0;                                 // number of parts in phased vaccination
        nSizePrevaccinateAge = 0;
        nMaxInfectionParity = NUM_OF_SEROTYPES;
        expansionFactor = 1;

        for (int i=0; i<NUM_OF_SEROTYPES; i++) {
            nInitialExposed[i]=0;
            nInitialInfected[i]=0;
            nDailyExposed[i]=0.0;
        }
        fPrimaryPathogenicity.clear();
        fPrimaryPathogenicity.resize(NUM_OF_SEROTYPES, 1.0);
        fPrimaryPathogenicity[1] = fPrimaryPathogenicity[3] = 0.25;

        fSecondaryScaling.clear();
        fSecondaryScaling.resize(NUM_OF_SEROTYPES, 1.0);

        //std::cerr << "Dengue model, Version " << VERSION_NUMBER_MAJOR << "." << VERSION_NUMBER_MINOR << std::endl;
        //std::cerr << "written by Dennis Chao and Thomas Hladish in 2012" << std::endl;

        if (argc>1) {
            for (int i=1; i<argc; i++) {
                char **end = NULL;
                if (strcmp(argv[i], "-randomseed")==0) {
                    randomseed = strtol(argv[i+1],end,10);
                    i++;
                }
                else if (strcmp(argv[i], "-runlength")==0) {
                    nRunLength = strtol(argv[i+1],end,10);
                    i++;
                }
                else if (strcmp(argv[i], "-initialinfected")==0) {
                    for (int j=0; j<NUM_OF_SEROTYPES; j++)
                        nInitialInfected[j]=strtol(argv[i+1+j],end,10);
                    i+=NUM_OF_SEROTYPES;
                }
                else if (strcmp(argv[i], "-initialexposed")==0) {
                    for (int j=0; j<NUM_OF_SEROTYPES; j++)
                        nInitialExposed[j]=strtol(argv[i+1+j],end,10);
                    i+=NUM_OF_SEROTYPES;
                }
                else if (strcmp(argv[i], "-dailyexposed")==0) {
                    for (int j=0; j<NUM_OF_SEROTYPES; j++)
                        nDailyExposed[j]=strtod(argv[i+1+j],end);
                    i+=NUM_OF_SEROTYPES;
                }
                else if (strcmp(argv[i], "-primarypathogenicity")==0) {
                    for (int j=0; j<NUM_OF_SEROTYPES; j++)
                        fPrimaryPathogenicity[j]=strtod(argv[i+1+j],end);
                    i+=NUM_OF_SEROTYPES;
                }
                else if (strcmp(argv[i], "-secondaryscaling")==0) {
                    for (int j=0; j<NUM_OF_SEROTYPES; j++)
                        fSecondaryScaling[j]=strtod(argv[i+1+j],end);
                    i+=NUM_OF_SEROTYPES;
                }
                else if (strcmp(argv[i], "-betapm")==0) {
                    betaPM = strtod(argv[i+1],end);
                    i++;
                }
                else if (strcmp(argv[i], "-betamp")==0) {
                    betaMP = strtod(argv[i+1],end);
                    i++;
                }
                else if (strcmp(argv[i], "-expansionfactor")==0) {
                    expansionFactor = strtod(argv[i+1],end);
                    i++;
                }
                else if (strcmp(argv[i], "-mosquitomove")==0) {
                    fMosquitoMove = strtod(argv[i+1],end);
                    i++;
                }
                else if (strcmp(argv[i], "-mosquitomovemodel")==0) {
                    szMosquitoMoveModel = argv[i+1];
                    i++;
                }
                else if (strcmp(argv[i], "-mosquitoteleport")==0) {
                    fMosquitoTeleport = strtod(argv[i+1],end);
                    i++;
                }
                else if (strcmp(argv[i], "-mosquitocapacity")==0) {
                    nDefaultMosquitoCapacity = strtol(argv[i+1],end,10);
                    i++;
                }
                else if (strcmp(argv[i], "-mosquitodistribution")==0) {
                    if (strcmp(argv[i+1], "constant")==0)
                        eMosquitoDistribution = CONSTANT;
                    else if (strcmp(argv[i+1], "exponential")==0)
                        eMosquitoDistribution = EXPONENTIAL;
                    else {
                        std::cerr << "ERROR: Invalid mosquito distribution specified." << std::endl;
                        exit(-1);
                    }
                    i++;
                }
                else if (strcmp(argv[i], "-mosquitomultipliers")==0) {
                    nSizeMosquitoMultipliers = strtol(argv[i+1],end,10);
                    assert(nSizeMosquitoMultipliers<54);
                    i++;
                    nMosquitoMultiplierCumulativeDays[0] = 0;
                    for (int j=0; j<nSizeMosquitoMultipliers; j++) {
                        nMosquitoMultiplierDays[j] = strtol(argv[i+1],end,10);
                        nMosquitoMultiplierCumulativeDays[j+1] =
                            nMosquitoMultiplierCumulativeDays[j]+nMosquitoMultiplierDays[j];
                        i++;
                        fMosquitoMultipliers[j] = strtod(argv[i+1],end);
                        i++;
                    }
                }
                else if (strcmp(argv[i], "-externalincubations")==0) {
                    nSizeExternalIncubation = strtol(argv[i+1],end,10);
                    assert(nSizeExternalIncubation<54);
                    i++;
                    nExternalIncubationCumulativeDays[0] = 0;
                    for (int j=0; j<nSizeExternalIncubation; j++) {
                        nExternalIncubationDays[j] = strtol(argv[i+1],end,10);
                        nExternalIncubationCumulativeDays[j+1] =
                            nExternalIncubationCumulativeDays[j]+nExternalIncubationDays[j];
                        i++;
                        nExternalIncubation[j] = strtol(argv[i+1],end,10);
                        assert(nExternalIncubation[j]<MAX_MOSQUITO_INCUBATION);
                        assert(nExternalIncubation[j]>0);
                        i++;
                    }
                }
                else if (strcmp(argv[i], "-vaccinatephased")==0) {
                    nSizeVaccinate = strtol(argv[i+1],end,10);
                    i++;
                    for (int j=0; j<nSizeVaccinate; j++) {
                        nVaccinateYear[j] = strtol(argv[i+1],end,10);
                        i++;
                        nVaccinateAge[j] = strtol(argv[i+1],end,10);
                        i++;
                        fVaccinateFraction[j] = strtod(argv[i+1],end);
                        i++;
                    }
                }
                else if (strcmp(argv[i], "-daysimmune")==0) {
                    nDaysImmune = strtol(argv[i+1],end,10);
                    i++;
                }
                else if (strcmp(argv[i], "-maxinfectionparity")==0) {
                    nMaxInfectionParity = strtol(argv[i+1],end,10);
                    i++;
                    assert(nMaxInfectionParity>0 && nMaxInfectionParity<=NUM_OF_SEROTYPES);
                }
                else if (strcmp(argv[i], "-VES")==0 || strcmp(argv[i], "-ves")==0) {
                    fVESs.clear();
                    fVESs.resize(4, strtod(argv[i+1],end));
                    i++;
                }
                else if (strcmp(argv[i], "-VESs")==0 || strcmp(argv[i], "-vess")==0) {
                    // different VES for each serotype
                    fVESs[0] = strtod(argv[i+1],end);
                    fVESs[1] = strtod(argv[i+2],end);
                    fVESs[2] = strtod(argv[i+3],end);
                    fVESs[3] = strtod(argv[i+4],end);
                    i+=4;
                }
                else if (strcmp(argv[i], "-VEI")==0 || strcmp(argv[i], "-vei")==0) {
                    fVEI = strtod(argv[i+1],end);
                    i++;
                }
                else if (strcmp(argv[i], "-VEP")==0 || strcmp(argv[i], "-vep")==0) {
                    fVEP = strtod(argv[i+1],end);
                    i++;
                }
		else if (strcmp(argv[i], "-vaccineleaky")==0) { // -dlc
		  bVaccineLeaky=true;
                }
                else if (strcmp(argv[i], "-prevaccinate")==0) {
                    fPreVaccinateFraction = strtod(argv[i+1],end);
                    i++;
                }
                else if (strcmp(argv[i], "-prevaccinateage")==0) {
                    nSizePrevaccinateAge = strtol(argv[i+1],end,10);
                    i++;
                    for (int j=0; j<nSizePrevaccinateAge; j++) {
                        nPrevaccinateAgeMin[j] = strtol(argv[i+1],end,10);
                        i++;
                        nPrevaccinateAgeMax[j] = strtol(argv[i+1],end,10);
                        i++;
                        fPrevaccinateAgeFraction[j] = strtod(argv[i+1],end);
                        i++;
                    }
                }
                else if (strcmp(argv[i], "-nosecondary")==0) {
                    bSecondaryTransmission = false;
                }
                else if (strcmp(argv[i], "-popfile")==0) {
                    szPopulationFile = argv[i+1];
                    i++;
                }
                else if (strcmp(argv[i], "-immfile")==0) {
                    szImmunityFile = argv[i+1];
                    i++;
                }
                else if (strcmp(argv[i], "-locfile")==0) {
                    szLocationFile = argv[i+1];
                    i++;
                }
                else if (strcmp(argv[i], "-netfile")==0) {
                    szNetworkFile = argv[i+1];
                    i++;
                }
                else if (strcmp(argv[i], "-peoplefile")==0) {
                    szPeopleFile = argv[i+1];
                    i++;
                }
                else if (strcmp(argv[i], "-yearlypeoplefile")==0) {
                    szYearlyPeopleFile = argv[i+1];
                    i++;
                }
                else if (strcmp(argv[i], "-dailyfile")==0) {
                    szDailyFile = argv[i+1];
                    i++;
                }
                else if (strcmp(argv[i], "-probfile")==0) {
                    szSwapProbFile = argv[i+1];
                    i++;
                }
                else {
                    std::cerr << "Unknown option: " << argv[i] << std::endl;
                    std::cerr << "Check arguments and formatting." << std::endl;
                    exit(-1);
                }
            }
        }
        //std::cerr << "population file = " << szPopulationFile << std::endl;
        //std::cerr << "immunity file = " << szImmunityFile << std::endl;
        //std::cerr << "location file = " << szLocationFile << std::endl;
        //std::cerr << "network file = " << szNetworkFile << std::endl;
        //std::cerr << "swap probabilities file = " << szSwapProbFile << std::endl;
        //std::cerr << "runlength = " << nRunLength << std::endl;
        if (nRunLength>MAX_RUN_TIME) {
            std::cerr << "ERROR: runlength is too long: " << nRunLength << std::endl;
            std::cerr << " change Parameters.h and recompile." << std::endl;
            exit(-1);
        }
        if (nRunLength==365) {
            //std::cerr << "WARNING: you probably want runlength to be 364, not 365" << std::endl;
        }
        //std::cerr << "random seed = " << randomseed << std::endl;
        //std::cerr << "beta_PM = " << betaPM << std::endl;
        //std::cerr << "beta_MP = " << betaMP << std::endl;
        //std::cerr << "days of complete cross protection = " << nDaysImmune << std::endl;
        //std::cerr << "maximum infection parity = " << nMaxInfectionParity << std::endl;
        //std::cerr << "pathogenicity of primary infection =";
        for (int i=0; i<NUM_OF_SEROTYPES; i++) {
            //std::cerr << " " << fPrimaryPathogenicity[i];
        }
        //std::cerr << std::endl;
        //std::cerr << "pathogenicity scaling for secondary infection =";
        for (int i=0; i<NUM_OF_SEROTYPES; i++) {
            //std::cerr << " " << fSecondaryScaling[i];
        }
        //std::cerr << std::endl;
        //std::cerr << "mosquito move prob = " << fMosquitoMove << std::endl;
        //std::cerr << "mosquito move model = " << szMosquitoMoveModel << std::endl;
        if ( szMosquitoMoveModel != "uniform" and szMosquitoMoveModel != "weighted" ) {
            std::cerr << "ERROR: invalid mosquito movement model requested:" << std::endl;
            std::cerr << " -mosquitomovemodel may be uniform or weighted " << nRunLength << std::endl;
            exit(-1);
        }
        //std::cerr << "mosquito teleport prob = " << fMosquitoTeleport << std::endl;
        //std::cerr << "default mosquito capacity per building = " << nDefaultMosquitoCapacity << std::endl;
        //std::cerr << "number of daily exposures =";
        for (int i=0; i<NUM_OF_SEROTYPES; i++) {
            //std::cerr << " " << nDailyExposed[i];
        }
        //std::cerr << std::endl;
	if (eMosquitoDistribution==CONSTANT) {
	  //std::cerr << "mosquito capacity distribution is constant" << std::endl;
	} else if (eMosquitoDistribution==EXPONENTIAL)
	  //std::cerr << "mosquito capacity distribution is exponential" << std::endl;
        if (nSizeMosquitoMultipliers>0) {
            //std::cerr << "mosquito seasonal multipliers (days,mult) =";
            for (int j=0; j<nSizeMosquitoMultipliers; j++) {
                //std::cerr << " (" << nMosquitoMultiplierDays[j] << "," << fMosquitoMultipliers[j] << ")";
            }
            //std::cerr << std::endl;
        }
        if (nSizeExternalIncubation>0) {
            //std::cerr << "external incubation periods (days,EIP) =";
            for (int j=0; j<nSizeExternalIncubation; j++) {
                //std::cerr << " (" << nExternalIncubationDays[j] << "," << nExternalIncubation[j] << ")";
            }
            //std::cerr << std::endl;
        }
        //std::cerr << "Pre-vaccinate fraction = " << fPreVaccinateFraction << std::endl;
        if (nSizeVaccinate>0) {
            //std::cerr << "Phased vaccinate (year, age, frac) = ";
            for (int j=0; j<nSizeVaccinate; j++) {
                //std::cerr << " (" << nVaccinateYear[j] << "," << nVaccinateAge[j]  << "," << fVaccinateFraction[j] << ")";
            }
            //std::cerr << std::endl;
        }
        if (nSizePrevaccinateAge>0) {
            //std::cerr << "Pre-vaccinate by age (min, max, frac) = ";
            for (int j=0; j<nSizePrevaccinateAge; j++) {
                //std::cerr << " (" << nPrevaccinateAgeMin[j] << "," << nPrevaccinateAgeMax[j]  << "," << fPrevaccinateAgeFraction[j] << ")";
            }
            //std::cerr << std::endl;
        }
        //std::cerr << "VE_I = " << fVEI << std::endl;
        //std::cerr << "VE_P = " << fVEP << std::endl;
        if (fVEI>1.0 || fVEP>1.0) {
            std::cerr << "ERROR: VE_S, VE_I, and VE_P must be between 0 and 1" << std::endl;
            exit(-1);
        }
        //std::cerr << "VE_Ss = " << fVESs[0] << "," << fVESs[1] << "," << fVESs[2] << "," << fVESs[3] << std::endl;

	if (bVaccineLeaky) {
	  //std::cerr << "VE_S is leaky" << std::endl;
	} else {
	  //std::cerr << "VE_S is all-or-none" << std::endl;
    }

    if (szPeopleFile.length()>0) {
        //std::cerr << "people output file = " << szPeopleFile << std::endl;
    } else {
        //std::cerr << "no people output file" << std::endl;
    }
    if (szYearlyPeopleFile.length()>0) {
        //std::cerr << "yearly people output file = " << szYearlyPeopleFile << std::endl;
    }
    if (szDailyFile.length()>0) {
        //std::cerr << "daily output file = " << szDailyFile << std::endl;
    } else {
        //std::cerr << "no daily output file" << std::endl;
    }
}
