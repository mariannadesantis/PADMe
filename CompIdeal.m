function fxk = CompIdeal(solint,data)

% This function computes the second component of the ideal point (IDk)
% of the k-th slice problem (N.B: the first component of IDk is obtained 
% by solving MILPk):
%
% INPUT: 
%       solint = file containing the integer fixings related to the slice
%         problem - obtained by solving MILPk;
%       data = file containing the number of integer and continuous
%         variables and the coefficients of the second objective function
%         (objective function of the slice problem).
%
% OUTPUT: 
%       fxk = second component of the ideal point (IDk) of the k-th slice
%       problem.

% Delete the LP created at the previous iteration:
delete newID.mps

% Read from "data" the #integer and continuous variables (nint,ncont)
fileID2 = fopen(data,'r');
ncont = str2num(fgetl(fileID2));
nint = str2num(fgetl(fileID2));
nvar = ncont + nint;
% Read from "data" the coefficients of the second objective function (f2)
f2 = zeros(ncont+nint,0);
for i = 1:ncont+nint
    f2(i) = str2num(fgetl(fileID2));
end

fclose(fileID2);

% Read from "solint" the integer fixings (xint) related to the current 
% slice problem, obtained by solving MILPk
xint = zeros(nint,1);
fileID3 = fopen(solint,'r');
for i = 1:nint
 xint(i) = str2num(fgetl(fileID3));
end

fclose(fileID3);

% Compute "fxk" by solving the LP obtained from the slice problem 
% (i.e. imposing the integer fixings xint) and having f2 as o.f. 

% Build the LP and solve it with GUROBI: 

% 1) append starting part of the mps file (N.B. INIT2 is created by writemps.m)
st1 = fileread('INIT2.mps');
[fid,msg] = fopen('newID.mps','at');
assert(fid>=3,msg)
fprintf(fid,st1);
fclose(fid);
 
fileID = fopen('newID.mps','a+');

% 2) impose the fixings 
fprintf(fileID,'%s\r\n','BOUNDS');
for i = 1:nvar
    if (i<=ncont)
    else
        fprintf(fileID,' %s','FX BND');
        fprintf(fileID,'\t%s%d','x',i);
        fprintf(fileID,'\t%d\r\n',xint(i-ncont));
    end
end

fprintf(fileID,'%s','ENDATA');
fclose(fileID);

copyfile('newID.mps','lpk2.mps')

% 3) solve the LP and store the solution in "id2k.txt"
fileIDout = fopen('id2k.txt','w');
model0 = gurobi_read('lpk2.mps');

% Write the .lp from the .mps (necessary for GUROBI):
gurobi_write(model0, 'lpk2.lp');
model = gurobi_read('lpk2.lp');
model.modelsense = 'min';

params.outputflag = 0;
result = gurobi(model, params);

if(result.status == 'OPTIMAL')
fxk = result.objval;
    fprintf(fileIDout,'%e\n', result.objval);
    fclose(fileIDout);
else
    fxk = 10000;
    fprintf(fileIDout,'%d\n',fxk);
end

if ~strcmp(result.status, 'OPTIMAL')
    fprintf('The LP model could not be solved because its optimization status is %s\n', result.status);
    return;
end

end
