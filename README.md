# PADMe
This repository contains an implementation of PADMe, the algorithm for bi-objective mixed integer linear problems presented in 
"On the accurate detection of the Pareto frontier for bi-objective mixed integer linear problems" by Lavinia Amorosi and Marianna De Santis

PADMe alternates the resolution of single-objective mixed integer linear problems (MILPs) with bi-objective continuous linear problems (BOLPs)
MILPs are solved using the MILP solver Gurobi, while BOLPs are solved using Bensolve.

Therefore, in order to run the code, you need to have installed:
 MATLAB (https://www.mathworks.com)
 Gurobi (https://www.gurobi.com)
 Bensolve (http://www.bensolve.org/)


The repositoty has the following Subdirectories

    1) src : in this subdirectory you find the source code of PADMe. 

    2) data : in this subdirectory you find the data files needed for the experiments.

    3) scripts : in this subdirectory you find the the scripts to replicate the experiments in the paper.

    4) results : in this subdirectory you find the plots from the paper.



