This folder contains the instances used in the computational experiments discussed in the paper.

They have been taken from the repository linked to the paper by Natashia Boland, Hadi Charkhgard, and Martin Savelsbergh. 
"A criterion space search algorithm for biobjective mixed integer programming: The triangle splitting method." 
INFORMS Journal on Computing, 27(4):597–618, 2015. 


Each data file contains the
following information in this order:

• Number of constraints

• Number of continuous decision variables

• Number of binary decision variables

• First objective function coefficients (first enter coefficients of continuous variables and then those of binary variables)

• Second objective function coefficients (first enter coefficients of continuous variables and then those of binary variables)

• Constraints coefficient matrix for continuous variables (for each row, meaning constraint, enter coefficients of continuous variables)

• Constraints coefficient matrix for binary variables (for each row, meaning constraint, enter coefficients of binary variables)

• Right hand side value of each constraint

• A binary value indicating the type of constraint. A zero means an = constraint and a one means a ≤ constraint.

