# PADMe
This repository contains an implementation of PADMe, the algorithm for bi-objective mixed integer linear problems presented in 
"On the accurate detection of the Pareto frontier for bi-objective mixed integer linear problems" by Lavinia Amorosi and Marianna De Santis

In order to run the code, you need to have installed:
 MATLAB (https://www.mathworks.com)
 Gurobi (https://www.gurobi.com)
 Bensolve (http://www.bensolve.org/)

The repository contains the following files:

1) ... : the main file (in C++)
   
2) ... : the header file
   
3) writeMPS.m : a MATLAB function that reads the BOMILP instance and writes some .mps files needed to build the MILPs
   and LPs addressed along the iterations of PADMe.

4) updateMPS.m : a MATLAB function that builds the problem MILPk in .mps format at each iteration of PADMe.

5) milpk.m : a MATLAB function that solves problem MILPk, detecting the new slice problem to consider

6) CompIdeal.m : a MATLAB function that computes the second component of the ideal point (IDk)
   of the k-th slice problem

7) A binch of .sh files needed to call the MATLAB functions from the C++ main file
   

To run the code, you need to properly address the libraries within the main C++ file ...
