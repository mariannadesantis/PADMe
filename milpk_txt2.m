function [xk,fxk,zID1k] = milpk_txt2(str,data)

% This function solves problem MILPk, detecting the new slice problem to 
%  consider at iteration (k), (i.e. the integer fixings to consider 
%  at iteration k) and the first component of its ideal point.
%
% INPUT: 
%       str = problem MILPk in .mps format, build by updateMPS.m;
%       data = file containing the number of integer and continuous
%         variables and the coefficients of the second objective function.
%
% OUTPUT: 
%       xk = optimal solution of MILPk;
%       fxk = second objective function evaluated at xk (fxk);
%       zID1k = first component of the ideal point (IDk) of the k-th slice
%        problem;
%       file solint_new.txt = a text file containing the integer fixings 
%        of the k-th slice problem and fxk;
%       file yk.txt = a text file containing the first component of the
%        ideal point of the k-th slice problem.


fileID = fopen('solint_new.txt','w');
filef1 = fopen('yk.txt','w');

% Read from "data" the #integer and continuous variables (nint,ncont)
fileID2 = fopen(data,'r');
ncont = str2num(fgetl(fileID2));
nint = str2num(fgetl(fileID2));
% Read from "data" the coefficients of the second objective function (f2)
f2 = zeros(ncont+nint,0);
for i = 1:ncont+nint
    f2(i) = str2num(fgetl(fileID2));
end

% Solve MILPk:

% 1) Read MILPk in .mps format
model0 = gurobi_read(str);
% Write the .lp from the .mps (necessary for GUROBI):
gurobi_write(model0, 'milpk.lp');
model = gurobi_read('milpk.lp');
model.modelsense = 'min';

params.outputflag = 0;
result = gurobi(model, params);

% 2) Store into solint_new.txt the integer variables of the optimal 
%     solution (xk) of MILPk and the value of the second objective function 
%     evaluated at xk (fxk) 
% 3) Store into yk.txt the optimal value of MILPk, i.e. the first
%     component of the ideal point of the k-th slice problem (zID1k)
xk = zeros(ncont+nint,1);
if strcmp(result.status, 'OPTIMAL')
    fxk = 0;
    zID1k = 0;
    for j = 1:ncont+nint
        xk(j) = result.x(find(model.varnames == "x"+j));
        fxk = fxk + f2(j)*xk(j);
        zID1k = result.objval;
        if(j>=ncont+1)
            fprintf(fileID,'%g\n', result.x(find(model.varnames == "x"+j)));
        end
    end
    fprintf(fileID,'%d\n',fxk);
    fprintf(filef1,'%d\n',zID1k);
    fprintf('Obj: %e\n', result.objval);
    fclose(fileID)
else
    fxk = 10000;
    fprintf(fileID,'%d\n',fxk);
    fprintf(filef1,'%d\n',fxk);
end

if ~strcmp(result.status, 'OPTIMAL')
    fprintf('This model cannot be solved because its optimization status is %s\n', result.status);
    return;
end

end
