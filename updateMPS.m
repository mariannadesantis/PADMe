function [xk,fxk] = updateMPS(it,solint,data)

% This function builds the problem MILPk in .mps format. 
% N.B. MILPk is the MILP problem that minimizes the first objective function 
% over a bunch of no-good constraints and a constraint related to the second
% objective function (f2): such contraints are needed to exclude the slice 
% problems encountered so far and those dominated by f2.
% The .mps file containing MILPk is built updating the .mps files 
% ROWS.mps, COLUMNS.mps and RHS.mps.
%
% INPUT: 
%       it = file containing the number of iterations done and the number
%        of constraints of the original problem;
%       solint = file containing the integer fixings related to the 
%        previous slice problem and the value of the second objective 
%        function evaluated over the optimal solution of MILP^(k-1);
%       data = file containing the number of integer and continuous
%         variables and the coefficients of the second objective function.
%
% OUTPUT: 
%       xk = optimal solution of MILPk;
%       fxk = second objective function evaluated at xk (fxk);
%       file  milpk.mps = the .mps file containing MILPk
%       

delete new.mps %!!!! SPOSTARE ALLA FINE?!

% Read from "it" the current number of iterations and the number of constraints 
% (needed in order to know how many no-good constraints have been 
%  added so far into MILPk (k+m)):
fileID3 = fopen(it,'r');
k = str2num(fgetl(fileID3));
m = str2num(fgetl(fileID3));

% Read from "data" the #integer and continuous variables (nint,ncont)
fileID2 = fopen(data,'r');
ncont = str2num(fgetl(fileID2));
nint = str2num(fgetl(fileID2));


% Read from "solint" the integer fixings (xint) related to the previous
% slice problem (obtained by solving MILP^(k-1)) and the value of the
% second objective function over the optimal solution of MILP^(k-1) (f2):
% These info are needed to write the new NO-GOOD constraint and the
% constraint f2(x)<= f2(x^{k-1}) into MILP^k.
xint = zeros(nint,1);
fileID = fopen(solint,'r');
for i = 1:nint
 xint(i) = str2num(fgetl(fileID));
end
f2 = str2num(fgetl(fileID));
 
% Build the .mps file for MILPk starting from the .mps files for MILP^(k-1)

% 1) Update the ROW part of the .mps file that models MILP^(k-1)  
copyfile('ROWS.mps','app.mps')
fileID = fopen('app.mps','a+');

% 2) Add the new ROW, related to the new NO-GOOD constraint: L c m+1+k
fprintf(fileID,' %s','L ');
fprintf(fileID,' %s%d\n','c',m+1+k);
copyfile('app.mps','ROWS.mps')

% 3) Update the COLUMN part of the .mps file that models MILP^(k-1)
copyfile('COLUMNS.mps','app.mps')
fileID = fopen('app.mps','a+');

% 4) Add the new NO-GOOD constraint (binary case)
a = zeros(length(xint),1);
b = -1;
for i =1:length(xint)
    if(xint(i)==1)
        a(i) = 1;
        b = b+1;
    else
        a(i) = -1;
    end
end

for i = 1:nint
    j = ncont+ i;
    fprintf(fileID,' %s%d','x',j);
    numadd = 0;
    if (numadd < 2)
        if(a(i)~=0)
            fprintf(fileID,'\t%s%d\t%d','c',m+1+k,a(i));
            numadd = numadd + 1;
        end
    else
        if(a(i)~=0)
            fprintf(fileID,'\r\n');
            fprintf(fileID,' %s%d','x',j);
            fprintf(fileID,'\t%s%d\t%d','c',m+1+k,a(i));
            numadd = 1;
        end
    end
    fprintf(fileID,'\r\n');
end
copyfile('app.mps','COLUMNS.mps');

% 5) Update the RHS part of the .mps file that models MILP^(k-1)
copyfile('RHS.mps','app.mps');
fileID = fopen('app.mps','a+');
% 6) Add the RHS of the new NO-GOOD constraint
fprintf(fileID,' %s','rhs');
fprintf(fileID,'\t%s%d\t%d\n','c',m+1+k,b);
copyfile('app.mps','RHS.mps')
 
% 7) Append the ROW, COLUMN, RHS part of the file
st1 = fileread('ROWS.mps');
st2 = fileread('COLUMNS.mps');
st3 = fileread('RHS.mps');
[fid,msg] = fopen('new.mps','at');
assert(fid>=3,msg)
fprintf(fid,st1);
fprintf(fid,st2);
fprintf(fid,st3);
fclose(fid);
 
fileID = fopen('new.mps','a+');

% 8) Update the rhs of the f2(x)<= f2(x^{k-1}) constraint 
fprintf(fileID,' %s','rhs');
fprintf(fileID,'\t%s%d\t%d\n','c',m+1,f2);


% 9) Append the last part of the .mps file (that does not evolve 
% along the iterations)
st2 = fileread('END.mps');
[fid,msg] = fopen('new.mps','at');
assert(fid>=3,msg)
fprintf(fid,st2);
fclose(fid);

% Copy the entire new.mps file into milpk.mps
copyfile('new.mps','milpk.mps')

%solve MILPk with GUROBI:
[xk,fxk] = milpk_txt2('milpk.mps','data.txt');
