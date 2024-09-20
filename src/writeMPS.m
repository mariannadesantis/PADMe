function [ncont,nint,m,f2] = writeMPS(str)

% This function reads the BOMILP instance that needs to be solved and writes 
% some .mps files needed to build the MILPs and LPs addressed along the 
% iterations of PADMe.
%   N.B: we used the format adopted in the paper presenting the 
%        Triangle Splitting Method: [Boland et al. IJOC 27(4):597–618,2015] 
%        The instances can be downloaded from 
%        https://usf.app.box.com/s/6i7rcdd7njkqsvnpi7x97ku9og3j8782
%
% INPUT: 
%       str = file containing the instance data.
%
% OUTPUT: 
%       ncont = number of continuous variables of the instance;
%       nint = number of integer variables of the instance;
%       m = number of constraints of the instance;
%       f2 = coefficients of the second objective function;
%       file 1_1or.mps = file .mps containing the MILP obtained from the
%         original BOMILP with the first o.f. as objective function;
%       file 1_2or.mps = file .mps containing the MILP obtained from the
%         original BOMILP with the second o.f. as objective function;
%       file ROWS.mps = file .mps needed to build MILPk along the
%         iterations of PADMe: at the end of the execution of this function 
%         it contains the part of 1_1or.mps related to the constraints;
%       file COLUMNS.mps = file .mps needed to build MILPk along the
%         iterations of PADMe: at the end of the execution of this function 
%         it contains the part of 1_1or.mps related to the variables;
%       file RHS.mps = file .mps needed to build MILPk along the
%         iterations of PADMe: at the end of the execution of this function 
%         it contains the part of 1_1or.mps related to the right hand sides 
%         of the constraints;
%       file END.mps = file .mps needed to build MILPk along the
%         iterations of PADMe: it containts the last line of the .mps file;
%       file INIT2.mps = file .mps needed to build the LP file for the
%         computation of the ideal point along the iteration (in the ID 
%         STRATEGY framework)
%       file data.txt = file containing the number of continuous and 
%         integer variables and the coefficients of the second objective 
%         function
%       file z1.txt = file containing the value of the first objective
%         function evaluated over x^{id,2} (reference value to stop PADMe)

fileID = fopen(str,'r');
% Read from "str" #constraints, #cont var, #int var
m = str2num(fgetl(fileID));
ncont = str2num(fgetl(fileID));
nint = str2num(fgetl(fileID));
nvar = ncont + nint;

% Open the .mps files that have to be written
mps1 = fopen('1_1or.mps','w');
mps1_init = fopen('ROWS.mps','w');
mps1_col = fopen('COLUMNS.mps','w');
mps1_rhs = fopen('RHS.mps','w');
mps1_end = fopen('END.mps','w');
mps2 = fopen('1_2or.mps','w');
mps2_init = fopen('INIT2.mps','w');
data = fopen('data.txt','w');


% Read from "str" the coefficients of the first objective function
f1cont = str2num(fgetl(fileID));
f1int = str2num(fgetl(fileID));
% Read from "str" the coefficients of the second objective function
f2cont = str2num(fgetl(fileID));
f2int = str2num(fgetl(fileID));
f1 = [f1cont f1int];
f2 = [f2cont f2int];

% Write in data.txt the number of continuous and integer variables and the
% coefficients of the second objective function
fprintf(data,'%d\n',ncont);
fprintf(data,'%d\n',nint);
for i = 1:ncont+nint
   fprintf(data,'%d\n',f2(i));
end

% Read from "str" the coefficients of the constraints:
% ...related to the continuous variables
for i = 1:m
    Acont(i,:) = str2num(fgetl(fileID));
end 
% ...related to the integer variables
for i = 1:m
    Aint(i,:) = str2num(fgetl(fileID));
end 
% Build the constraint matrix A
A = [Acont Aint];
% Read from "str" the right hand side of the constraints:
b = str2num(fgetl(fileID));
% Read from "str" the type of the constraints:
ty = str2num(fgetl(fileID));

% Build the .mps files:
fprintf(mps1,'%s\r\n','NAME    obj1');
fprintf(mps1_init,'%s\r\n','NAME    obj1');
fprintf(mps2,'%s\r\n','NAME    obj2');
fprintf(mps2_init,'%s\r\n','NAME    obj2');


fprintf(mps1,'%s\r\n','ROWS');
fprintf(mps1_init,'%s\r\n','ROWS');
fprintf(mps2,'%s\r\n','ROWS');
fprintf(mps2_init,'%s\r\n','ROWS');

fprintf(mps1,' %s\r\n','N obj');
fprintf(mps1_init,' %s\r\n','N obj');
fprintf(mps2,' %s\r\n','N obj');
fprintf(mps2_init,' %s\r\n','N obj');


% Define the constraints according to their type (<= or =):
for i = 1:m
    if (ty(i)==1)
        fprintf(mps1,' %s%d\r\n','L c',i);
        fprintf(mps1_init,' %s%d\r\n','L c',i);
        fprintf(mps2,' %s%d\r\n','L c',i);
        fprintf(mps2_init,' %s%d\r\n','L c',i);
    else
        fprintf(mps1,' %s%d\r\n','E c',i);
        fprintf(mps1_init,' %s%d\r\n','E c',i);
        fprintf(mps2,' %s%d\r\n','E c',i);
        fprintf(mps2_init,' %s%d\r\n','E c',i);
    end
end


% Add the further constraint f2(x) <= rhs (in order to change only the rhs 
% during the call of the algorithm) 
fprintf(mps1,' %s%d\r\n','L c',m+1);
fprintf(mps1_init,' %s%d\r\n','L c',m+1);



% Write the part of the .mps related to the variables:
fprintf(mps1,'%s\r\n','COLUMNS');
fprintf(mps1_col,'%s\r\n','COLUMNS');
fprintf(mps2,'%s\r\n','COLUMNS');
fprintf(mps2_init,'%s\r\n','COLUMNS');


for i = 1:nvar
    % first objective function coefficients
    fprintf(mps1,' %s%d','x',i);
    fprintf(mps1_col,' %s%d','x',i);
    fprintf(mps1,'\t%s\t%d\r\n','obj',f1(i));
    fprintf(mps1_col,'\t%s\t%d\r\n','obj',f1(i));
    % second objective function coefficients
    fprintf(mps2,' %s%d','x',i);
    fprintf(mps2_init,' %s%d','x',i);
    fprintf(mps2,'\t%s\t%d\r\n','obj',f2(i));
    fprintf(mps2_init,'\t%s\t%d\r\n','obj',f2(i));
    
    % constraints coefficients
    fprintf(mps1,' %s%d','x',i);
    fprintf(mps1_col,' %s%d','x',i);
    fprintf(mps2,' %s%d','x',i);
    fprintf(mps2_init,' %s%d','x',i);
    numadd = 0;
    for j = 1:m
        if (numadd < 2)
            if(A(j,i)~=0)
                fprintf(mps1,'\t%s%d\t%d','c',j,A(j,i));
                fprintf(mps1_col,'\t%s%d\t%d','c',j,A(j,i));
                fprintf(mps2,'\t%s%d\t%d','c',j,A(j,i));
                fprintf(mps2_init,'\t%s%d\t%d','c',j,A(j,i));
                numadd = numadd + 1;
            end
        else
            if(A(j,i)~=0)
                fprintf(mps1,'\r\n');
                fprintf(mps1_col,'\r\n');
                fprintf(mps2,'\r\n');
                fprintf(mps2_init,'\r\n');
                fprintf(mps1,' %s%d','x',i);
                fprintf(mps1_col,' %s%d','x',i);
                fprintf(mps2,' %s%d','x',i);
                fprintf(mps2_init,' %s%d','x',i);
                fprintf(mps1,'\t%s%d\t%d','c',j,A(j,i));
                fprintf(mps1_col,'\t%s%d\t%d','c',j,A(j,i));
                fprintf(mps2,'\t%s%d\t%d','c',j,A(j,i));
                fprintf(mps2_init,'\t%s%d\t%d','c',j,A(j,i));
                numadd = 1;
            end
        end
    end
    fprintf(mps1,'\r\n');
    fprintf(mps1_col,'\r\n');


    % Add the constraint f2(x)<= rhs
    if(f2(i)~=0)
        fprintf(mps1,' %s%d','x',i);
        fprintf(mps1,'\t%s%d\t%d','c',m+1,f2(i));
        fprintf(mps1_col,' %s%d','x',i);
        fprintf(mps1_col,'\t%s%d\t%d','c',m+1,f2(i));
    end
        
    fprintf(mps1,'\r\n');
    fprintf(mps2,'\r\n');
    fprintf(mps1_col,'\r\n');
    fprintf(mps2_init,'\r\n');
end


% Write the part of the .mps related to the right hand side of the constraints
fprintf(mps1,'%s\r\n','RHS');
fprintf(mps1_rhs,'%s\r\n','RHS');
fprintf(mps2,'%s\r\n','RHS');
fprintf(mps2_init,'%s\r\n','RHS');
for i = 1:m
    if(b(i)~=0)
        fprintf(mps1,' %s','rhs');
        fprintf(mps1_rhs,' %s','rhs');
        fprintf(mps2,' %s','rhs');
        fprintf(mps2_init,' %s','rhs');
        fprintf(mps1,'\t%s%d\t%d\n','c',i,b(i));
        fprintf(mps1_rhs,'\t%s%d\t%d\n','c',i,b(i));
        fprintf(mps2,'\t%s%d\t%d\n','c',i,b(i)); 
        fprintf(mps2_init,'\t%s%d\t%d\n','c',i,b(i)); 
    end
end


% Update the rhs of the f2(x)<= a constraint 
% (at the beginning we want a redundant constraint, so we set a = 10^6)
fprintf(mps1,' %s','rhs');
fprintf(mps1,'\t%s%d\t%d\n','c',m+1,10^6);
  
% Write the part of the .mps related to the bounds of the variables
fprintf(mps1,'%s\r\n','BOUNDS');
fprintf(mps1_end,'%s\r\n','BOUNDS');
fprintf(mps2,'%s\r\n','BOUNDS');
for i = 1:nvar
    if (i<=ncont)
    else
        fprintf(mps1,' %s','BV BOUND');
        fprintf(mps1_end,' %s','BV BOUND');
        fprintf(mps2,' %s','BV BOUND');
        fprintf(mps1,'\t%s%d\r\n','x',i);
        fprintf(mps1_end,'\t%s%d\r\n','x',i);
        fprintf(mps2,'\t%s%d\r\n','x',i);
    end
end


fprintf(mps1,'%s','ENDATA');
fprintf(mps1_end,'%s','ENDATA');
fprintf(mps2,'%s','ENDATA');

fclose(fileID);
fclose(mps1);
fclose(mps2);
fclose(mps2_init);


% Solve the MILP obtained from BOMILP having the second o.f. as objective
% function (1_2or.mps). 
% xID2 will be its optimal solution, z1ID the value of the first 
% objective function evaluated over xID2.
xId2 = zeros(ncont+nint,1);
z1Id = 0;
[xId2,~,~] = milpk_txt2('1_2or.mps','data.txt');
% Compute the value of z_1(x^{id,2})
for j = 1:ncont+nint
    z1Id = z1Id + f1(j)*xId2(j);
end
% Store into z1.txt the value of z_1(x^{id,2})
z1 = fopen('z1.txt','w');
fprintf(z1,'\t%d\n',z1Id);
fclose(z1);

end

 
