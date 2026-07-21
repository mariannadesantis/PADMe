#ifndef Header_h
#define Header_h

#include <sstream>
#include <fstream>
#include <cstring>
#include <iostream>
#include <string>
#include <list>
#include <iterator>
#include <vector>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>




//Function to print a vector of int
void print_vector(std::vector<double> vect)
{
    
    std::vector<double>::iterator v;
   

        for(v=vect.begin();v!=vect.end();++v)
            std::cout<<' '<<*v;
        
        std::cout<<"\n";
    
    return;
}

void matlab(std::string inputFile)
{
    std::string str;
    str="";
    str = str + inputFile;
    const char *command = str.c_str();
    system(command);
}

void update_z(std::string inputFile, std::vector<double> &z)
{
    std::fstream inputfile(inputFile);
    double M=0;
    while(inputfile)
        //std::cout<<"\n j="<<' '<<j<<"\n";
        inputfile>>M;
   // std::cout<<"\n z2 value:"<<M;
    z.push_back(M);
    
}


bool check_punto_ideale(double b, double v1, double v2, std::vector<double> punto_ideale)
{
    bool check=false;
    if(v1**punto_ideale.begin()+v2**std::next(punto_ideale.begin()) <= b)
        check=true;
    
    return check;
    
}


void update_it(std::string inputFile, std::string outputFile, int cont)
{
    std::fstream inputfile(inputFile);
    std::ofstream outfile(outputFile);
    
    double M=0;
    while(inputfile)
        //std::cout<<"\n j="<<' '<<j<<"\n";
        inputfile>>M;
   // std::cout<<"\n Last line of it.txt"<<M<<"\n";
   // std::cout<<"\n Counter value"<<cont<<"\n";
    outfile<<cont<<std::endl;
    outfile<<M<<std::endl;
    
    outfile.close();
}


void read_file(std::string inputFile, int &rows, int &cols, int &bv)
{
    std::fstream inputfile(inputFile);
    if (inputfile.is_open())
    {
        inputfile>>rows;
        inputfile>>cols;
        inputfile>>bv;
    }
    else std::cout << "Unable to open input file";
}
 
std::vector<double> store_data_int_fixing(std::string inputFile2, int *bvar, double *bnew, double *b, double &M, int bv, double **P, double **B2, double &z1int, double &z2int, int rows)
{
    
    std::vector<double> parti_intere;
    std::fstream inputfile2(inputFile2);
    for(int i=1;i<=bv;i++)
        bvar[i]=0;
    
    for(int j=1; j<=bv;j++)
    {
        
        //std::cout<<"\n j="<<' '<<j<<"\n";
        inputfile2>>*&bvar[j];
    }
    
    //Reading the optimal value of the second objective function for the current integer fixing
    //double M=0;
    inputfile2>>M;
   // std::cout<<"\n z2 value being used to write the vlp file:"<<M;
    
    //std::cout<<"\n Vettore fissaggio binarie";
    //for (int i=1; i<=bv; i++)
    //std::cout<<"\n"<<*&bvar[i];
    
    
    //Computing the integer component of the 2nd objective function
    z2int=0;
    for(int j=1; j<=bv;j++)
    {
        z2int=z2int+*&bvar[j]**&P[4][j];
    }
    
    //Computing the integer component of the 1st objective function
    z1int=0;
    for(int j=1; j<=bv;j++)
    {
        z1int=z1int+*&bvar[j]**&P[2][j];
    }
    
    //std::cout<<"\n Parte intera di z1:"<<z1int;
    
   // std::cout<<"\n Parte intera di z2:"<<z2int;
    
    parti_intere.push_back(z1int);
    parti_intere.push_back(z2int);
    
    
    //Building the updated b using the values of the binary variables in bvar
   // double bnew[rows+1];
    double bapp;
    
    for(int i=1;i<=rows;i++)
        bnew[i]=0;
    
    for (int i=1;i<=rows;i++)
    {
        bapp=0;
        for (int j=1; j<=bv;j++)
            bapp=bapp+B2[i][j]*bvar[j];
       // std::cout<<"\n"<<bapp;
        *&bnew[i]=b[i]-bapp;
    }
    
    return parti_intere;
    
}

void store_data(std::string inputFile, int bv, int *bvar, int rows, int cols, double **B1, double **B2, double **P, double *b, double *c, int &NB, int &NP1, int &NP2, int &NP, double *bnew, double &z1int, double &z2int)
{
    
    std::fstream inputfile(inputFile);
   
    if (inputfile.is_open())
    {
        int app;
        inputfile>>app;
        inputfile>>app;
        inputfile>>app;
        
        
        //Inizializzazione matrici B1, B2 e P
        for(int i=1;i<=rows;i++)
        B1[i]= new double[cols+1];
        
               // *(B1+i+j)=0;
              //  B1[i][j]=0;
            for(int i=1;i<=rows;i++)
            {
                for(int j=1; j<=cols;j++)
                    *&B1[i][j]=0;
            
        }
        
        for(int i=1;i<=rows;i++)
        B2[i]= new double[cols+1];
        
            for(int i=1;i<=rows;i++)
            {
            for(int j=1; j<=bv;j++)
                //*(B2+i+j)=0;
               *&B2[i][j]=0;
        }
        
        for(int i=1;i<=4;i++)
        P[i]= new double[cols+1];
        
        for(int i=1;i<=4;i++)
        {
            for(int j=1;j<=cols;j++)
                //*(P+i+j)=0;
               *&P[i][j]=0;
            
            
        }
        
        
        for(int i=1;i<=rows;i++)
        {
            *&b[i]=0;
            *&c[i]=0;
        }
        
        std::string line="";
        
        getline(inputfile,line);
        
        std::string::size_type lPos = line.find_first_not_of(" ", 0);
        std::string::size_type pos = line.find_first_of(" ", lPos);
        
        int i=1;
        while(i<=4)
        {
            //  std::cout<<"\n i="<<' '<<i<<"\n";
            getline(inputfile,line);
            //std::cout<<""<<line<<"\n";
            
            std::string::size_type lPos = line.find_first_not_of(" ", 0);
            std::string::size_type pos = line.find_first_of(" ", lPos);
            
            int j=1;
            while (std::string::npos != pos || std::string::npos != lPos)
            {
                
                //std::cout<<"\n j="<<' '<<j<<"\n";
                double v = atof(line.substr(lPos, pos - lPos).c_str());
                //std::cout<<""<<v<<"\n";
                //int v1=round(v);
                
                
                //Riempimento matrice P;
                *&P[i][j]=v;
               // *(P+i+j)=v;
                // std::cout<<""<<P[i][j]<<"\n";
                j++;
                lPos = line.find_first_not_of(" ", pos);
                pos = line.find_first_of(" ", lPos);
            }
            i++;
            // getline(inputfile,line);
        }
        
        i=1;
        while(i<=rows)
        {
            
            
            // std::cout<<"\n i="<<' '<<i<<"\n";
            getline(inputfile,line);
            //std::cout<<""<<line<<"\n";
            
            std::string::size_type lPos = line.find_first_not_of(" ", 0);
            std::string::size_type pos = line.find_first_of(" ", lPos);
            
            int j=1;
            while (std::string::npos != pos || std::string::npos != lPos)
            {
                
                // std::cout<<"\n j="<<' '<<j<<"\n";
                double v = atof(line.substr(lPos, pos - lPos).c_str());
                //Riempimento matrice B1
                *&B1[i][j]=v;
                // std::cout<<""<<B1[i][j]<<"\n";
                j++;
                lPos = line.find_first_not_of(" ", pos);
                pos = line.find_first_of(" ", lPos);
            }
            i++;
        }
        
        
        
        
        i=1;
        while(i<=rows)
        {
            
            // std::cout<<"\n i="<<' '<<i<<"\n";
            getline(inputfile,line);
            //std::cout<<""<<line<<"\n";
            
            std::string::size_type lPos = line.find_first_not_of(" ", 0);
            std::string::size_type pos = line.find_first_of(" ", lPos);
            
            int j=1;
            while (std::string::npos != pos || std::string::npos != lPos)
            {
                // std::cout<<"\n j="<<' '<<j<<"\n";
                double v = atof(line.substr(lPos, pos - lPos).c_str());
                //Riempimento matrice B2
                *&B2[i][j]=v;
                // std::cout<<""<<B2[i][j]<<"\n";
                j++;
                lPos = line.find_first_not_of(" ", pos);
                pos = line.find_first_of(" ", lPos);
            }
            
            i++;
            
        }
        
       /*
        std::cout<<"\n Stampa matrice B2 \n";
       
       for(int k=1;k<=rows;k++)
       {
           for(int m=1; m<=cols;m++)
           {
                 std::cout << ' ' << *&B2[k][m];
           
               
           }
           std::cout << '\n';
       }
       */
        
        getline(inputfile,line);
        
        
        lPos = line.find_first_not_of(" ", 0);
        pos = line.find_first_of(" ", lPos);
        
        int j=1;
      //  std::cout<<"\n";
        while (std::string::npos != pos || std::string::npos != lPos)
        {
            // std::cout<<"\n j="<<' '<<j<<"\n";
            double v = atof(line.substr(lPos, pos - lPos).c_str());
            //Riempimento vettore termini noti b
            *&b[j]=v;
            //std::cout<<""<<b[j]<<"\n";
            j++;
            lPos = line.find_first_not_of(" ", pos);
            pos = line.find_first_of(" ", lPos);
        }
      
        /*
        std::cout<<"\n Stampa vettore termini noti";
        for(int j=1; j<=rows;j++)
        std::cout<<"\n"<<b[j];
        */
        /*
        std::cout<<"\n Vettore fissaggio variabili";
        for(int l=1; l<=bv;l++)
            std::cout<<"\n"<<*&bvar[l];
       */
        //std::cout<<"\n Fine vettore fissaggio variabili";
        //Building the updated b using the values of the binary variables in bvar
       // double bnew[rows+1];
     /*   double bapp;
        
        for(int i=1;i<=rows;i++)
            bnew[i]=0;
        
        for (int i=1;i<=rows;i++)
        {
            bapp=0;
            for (int j=1; j<=bv;j++)
                bapp=bapp+B2[i][j]*bvar[j];
            std::cout<<"\n"<<bapp;
            *&bnew[i]=b[i]-bapp;
        }
    */
        /*
    std::cout<<"\n Vettore termini noti aggiornati";
        for(int i=1;i<=rows;i++)
            std::cout<<"\n"<<*&bnew[i];
        */
        
        getline(inputfile,line);
        //std::cout<<""<<line<<"\n";
        lPos = line.find_first_not_of(" ", 0);
        pos = line.find_first_of(" ", lPos);
        
        j=1;
        while (std::string::npos != pos || std::string::npos != lPos)
        {
            // std::cout<<"\n j="<<' '<<j<<"\n";
            double v = atof(line.substr(lPos, pos - lPos).c_str());
            //Riempimento vettore info verso vincoli
            *&c[j]=v;
            // std::cout<<""<<c[j]<<"\n";
            j++;
            lPos = line.find_first_not_of(" ", pos);
            pos = line.find_first_of(" ", lPos);
        }
        
        
        
       // int NB, NP1, NP2, NP;
        NB=NP1=NP2=NP=0;
        
        /*
        std::cout<<"\n Stampa matrice B1 \n";
        for(int k=1;k<=rows;k++)
        {
            for(int m=1; m<=cols;m++)
            {
                  std::cout << ' ' << *&B1[k][m];
                if(B1[k][m]!=0)
                    NB++;
                
            }
            std::cout << '\n';
        }
        */
        
         
        /*
        std::cout<<"\n Stampa matrice P \n";
        for(int i=1;i<=4;i++)
        {
            for(int j=1; j<=cols;j++)
            {
                std::cout << ' ' << *&P[i][j];
                if((i==1) && (P[i][j]!=0))
                    NP1++;
                if((i==3) && (P[i][j]!=0))
                    NP2++;
                
            }
            NP=NP1+NP2;
            std::cout << '\n';
        }
        
       */
        //Computing the integer component of the 2nd objective function
        z2int=0;
        for(int j=1; j<=bv;j++)
        {
            z2int=z2int+bvar[j]*P[4][j];
        }
        
        //Computing the integer component of the 1st objective function
        z1int=0;
        for(int j=1; j<=bv;j++)
        {
            z1int=z1int+bvar[j]*P[2][j];
        }
        
       // std::cout<<"\n Parte intera di z1:"<<z1int;
        
       // std::cout<<"\n Parte intera di z2:"<<z2int;
        
        
        std::vector<double> parti_intere;
        parti_intere.clear();
        parti_intere.push_back(z1int);
        parti_intere.push_back(z2int);
        
       // std::cout<<"\n Print integer-parts vector";
       // print_vector(parti_intere);
        //  std::cout<<"\n Print vector b \n";
        
        //    for(int j=1; j<=rows;j++)
        //    std::cout << ' ' << b[j];
        
        
        // std::cout<<"\n Print vector c \n";
        
        //   for(int j=1; j<=rows;j++)
        //  std::cout << ' ' << c[j];
        
    }
    return;
}

std::vector<double> build_vlp(std::string inputFile, std::string inputFile2, std::string outputFile)
    {
    std::vector<double> parti_intere;
    parti_intere.clear();
    
    std::fstream inputfile(inputFile);
    std::fstream inputfile2(inputFile2);
    std::ofstream outfile(outputFile);
    
    int rows, cols, bv;
    
    
    if (inputfile.is_open())
    {
        inputfile>>rows;
        inputfile>>cols;
        inputfile>>bv;
        
    
        
        int bvar[bv+1];
        
        for (int i=1; i<=bv; i++)
            bvar[i]=0;
        
       
        for(int j=1; j<=bv;j++)
        {
            
            //std::cout<<"\n j="<<' '<<j<<"\n";
            inputfile2>>bvar[j];
        }
        
        //Reading the optimal value of the second objective function for the current integer fixing
        double M=0;
        inputfile2>>M;
       // std::cout<<"\n z2 value being used to write the vlp file:"<<M;
        
       // std::cout<<"\n Vettore fissaggio binarie";
        //for (int i=1; i<=bv; i++)
          //  std::cout<<"\n"<<bvar[i];
        
       
        
        
       // std::cout<<"\n "<<rows;
       // std::cout<<"\n "<<cols;
       // std::cout<<"\n "<<bv;
        
        double B1[rows+1][cols+1]; //matrice dei coefficienti dei vincoli
        double B2[rows+1][bv+1]; //matrice dei coefficienti dei vincoli (variabili binarie)
        double P[4][cols+1]; //coefficient matrix (rows 1 and 3 are the continuous-variable rows) of the objective functions
        double b[rows+1]; //vettore termini noti vincoli
        double c[rows+1]; //vettore info forma vincoli (0 eq. 1 dis. <=)
        
        //Inizializzazione matrici B1, B2 e P
        for(int i=1;i<=rows;i++)
        {
            for(int j=1; j<=cols;j++)
                B1[i][j]=0;
            
        }
        for(int i=1;i<=rows;i++)
        {
            for(int j=1; j<=bv;j++)
             B2[i][j]=0;
        }
        
        for(int i=1;i<=2;i++)
        {
            for(int j=1;j<=cols;j++)
                
                P[i][j]=0;
            
            
        }
        
        
        for(int i=1;i<=rows;i++)
        {
            b[i]=0;
            c[i]=0;
        }
       
        std::string line="";
       
        getline(inputfile,line);
        
        std::string::size_type lPos = line.find_first_not_of(" ", 0);
        std::string::size_type pos = line.find_first_of(" ", lPos);
        
        int i=1;
        while(i<=4)
        {
          //  std::cout<<"\n i="<<' '<<i<<"\n";
            getline(inputfile,line);
            //std::cout<<""<<line<<"\n";
            
            std::string::size_type lPos = line.find_first_not_of(" ", 0);
            std::string::size_type pos = line.find_first_of(" ", lPos);
            
            int j=1;
            while (std::string::npos != pos || std::string::npos != lPos)
            {
                
                //std::cout<<"\n j="<<' '<<j<<"\n";
                double v = atof(line.substr(lPos, pos - lPos).c_str());
                //std::cout<<""<<v<<"\n";
                //int v1=round(v);
                
                
                //Riempimento matrice P;
                P[i][j]=v;
               // std::cout<<""<<P[i][j]<<"\n";
                j++;
                lPos = line.find_first_not_of(" ", pos);
                pos = line.find_first_of(" ", lPos);
            }
            i++;
           // getline(inputfile,line);
        }
        
        i=1;
        while(i<=rows)
        {
            
            
           // std::cout<<"\n i="<<' '<<i<<"\n";
            getline(inputfile,line);
            //std::cout<<""<<line<<"\n";
            
            std::string::size_type lPos = line.find_first_not_of(" ", 0);
            std::string::size_type pos = line.find_first_of(" ", lPos);
            
            int j=1;
            while (std::string::npos != pos || std::string::npos != lPos)
            {
                
               // std::cout<<"\n j="<<' '<<j<<"\n";
                double v = atof(line.substr(lPos, pos - lPos).c_str());
                //Riempimento matrice B1
                B1[i][j]=v;
               // std::cout<<""<<B1[i][j]<<"\n";
                j++;
                lPos = line.find_first_not_of(" ", pos);
                pos = line.find_first_of(" ", lPos);
            }
            i++;
        }
        
        
        
        
        i=1;
        while(i<=rows)
        {
            
           // std::cout<<"\n i="<<' '<<i<<"\n";
            getline(inputfile,line);
            //std::cout<<""<<line<<"\n";
            
            std::string::size_type lPos = line.find_first_not_of(" ", 0);
            std::string::size_type pos = line.find_first_of(" ", lPos);
            
            int j=1;
            while (std::string::npos != pos || std::string::npos != lPos)
            {
               // std::cout<<"\n j="<<' '<<j<<"\n";
                double v = atof(line.substr(lPos, pos - lPos).c_str());
                //Riempimento matrice B2
                B2[i][j]=v;
               // std::cout<<""<<B2[i][j]<<"\n";
                j++;
                lPos = line.find_first_not_of(" ", pos);
                pos = line.find_first_of(" ", lPos);
            }
            
            i++;
        
        }
        
        getline(inputfile,line);
       
        
        lPos = line.find_first_not_of(" ", 0);
        pos = line.find_first_of(" ", lPos);
        
        int j=1;
        while (std::string::npos != pos || std::string::npos != lPos)
        {
           // std::cout<<"\n j="<<' '<<j<<"\n";
            double v = atof(line.substr(lPos, pos - lPos).c_str());
            //Riempimento vettore termini noti b
            b[j]=v;
          //  std::cout<<""<<b[j]<<"\n";
            j++;
            lPos = line.find_first_not_of(" ", pos);
            pos = line.find_first_of(" ", lPos);
        }
        
        
        //Building the updated b using the values of the binary variables in bvar
        double bnew[rows+1];
        double bapp;
        
        for(int i=1;i<=rows;i++)
            bnew[i]=0;
        
        for (int i=1;i<=rows;i++)
        {
            bapp=0;
            for (j=1; j<=bv;j++)
            bapp=bapp+B2[i][j]*bvar[j];
            bnew[i]=b[i]-bapp;
        }
        
      //  std::cout<<"\nVettore termini noti aggiornati";
       // for(int i=1;i<=rows;i++)
         //   std::cout<<"\n"<<bnew[i];
        
        
        getline(inputfile,line);
        //std::cout<<""<<line<<"\n";
        lPos = line.find_first_not_of(" ", 0);
        pos = line.find_first_of(" ", lPos);
        
        j=1;
        while (std::string::npos != pos || std::string::npos != lPos)
        {
           // std::cout<<"\n j="<<' '<<j<<"\n";
            double v = atof(line.substr(lPos, pos - lPos).c_str());
            //Riempimento vettore info verso vincoli
            c[j]=v;
           // std::cout<<""<<c[j]<<"\n";
            j++;
            lPos = line.find_first_not_of(" ", pos);
            pos = line.find_first_of(" ", lPos);
        }
    
        

        int NB, NP1, NP2, NP;
        NB=NP1=NP2=NP=0;
        
      //  std::cout<<"\n Print matrix B1 \n";
        for(int k=1;k<=rows;k++)
        {
            for(int m=1; m<=cols;m++)
            {
              //  std::cout << ' ' << B1[k][m];
                if(B1[k][m]!=0)
                    NB++;
                
            }
          //  std::cout << '\n';
        }
        
        
       // std::cout<<"\n Print matrix B2 \n";
       
        
        
     //  std::cout<<"\n Print matrix P \n";
        for(int i=1;i<=4;i++)
        {
            for(int j=1; j<=cols;j++)
            {
               // std::cout << ' ' << P[i][j];
               if((i==1) && (P[i][j]!=0))
                    NP1++;
                if((i==3) && (P[i][j]!=0))
                    NP2++;
               
            }
          NP=NP1+NP2;
          //  std::cout << '\n';
        }
    
        
        //Computing the integer component of the 2nd objective function
        int z2int=0;
        for(int j=1; j<=bv;j++)
        {
            z2int=z2int+bvar[j]*P[4][j];
        }
        
        //Computing the integer component of the 1st objective function
        int z1int=0;
        for(int j=1; j<=bv;j++)
        {
            z1int=z1int+bvar[j]*P[2][j];
        }
        
      //  std::cout<<"\n Parte intera di z1:"<<z1int;
        
      //  std::cout<<"\n Parte intera di z2:"<<z2int;
        
        parti_intere.push_back(z1int);
        parti_intere.push_back(z2int);
        
       // std::cout<<"\n Print integer-parts vector";
       // print_vector(parti_intere);
      //  std::cout<<"\n Print vector b \n";
    
        //    for(int j=1; j<=rows;j++)
        //    std::cout << ' ' << b[j];
            
                
       // std::cout<<"\n Print vector c \n";
    
         //   for(int j=1; j<=rows;j++)
          //  std::cout << ' ' << c[j];
        
        
        
        
        
        int numObj=2;

        
        if (outfile.is_open()){
             
             outfile << "p vlp min" <<' '<<rows+1<<' '<<cols<<' '<<NB+NP2<<' '<<numObj<<' '<<NP<< std::endl;
             
            
             for (int i=1; i<=rows;i++)
             {
                 if(c[i]==0)
                 outfile <<"i"<<' '<<i<<'\t'<<"s"<<' '<<bnew[i]<< std::endl;
                 else if(c[i]==1)
                 outfile <<"i"<<' '<<i<<'\t'<<"u"<<' '<<bnew[i]<< std::endl;
             }
            
             
            //Additional constraint on the value of the second objective function
            outfile <<"i"<<' '<<rows+1<<'\t'<<"u"<<' '<<M-z2int<< std::endl;
            
           //Aggiungi
                
             for (int j=1; j<=cols;j++)
             {
                 
                    // outfile <<"j"<<' '<<j<<'\t'<<"f"<< std::endl; //variabili continue libere
                    outfile <<"j"<<' '<<j<<'\t'<<"l 0"<< std::endl; //variabili continue non negative
             }
             
             
             
             for(int i=1;i<=rows;i++)
             {
                 for(int j=1; j<=cols;j++)
                 {
                         if(B1[i][j]!=0)
                         outfile <<"a"<<' '<<i<<' '<<j<<' '<<B1[i][j]<< std::endl;
                     
                 }
                // std::cout << '\n';
               
             }
            
           
            
            for(int j=1; j<=cols;j++)
            {
                    if(P[3][j]!=0)
                    outfile <<"a"<<' '<<rows+1<<' '<<j<<' '<<P[3][j]<< std::endl;
                
            }
            
            
            
             //Writing the objective coefficients in the vlp file for two objectives
             for(int i=0;i<=2;i++)
            {
                
                 for(int j=1; j<=cols;j++)
                {

                     if((P[i+1][j]!=0)&&(i==0))
                         outfile <<"o"<<' '<<i+1<<' '<<j<<' '<<P[i+1][j]<< std::endl;
                     if((P[i+1][j]!=0)&&(i==2))
                        outfile <<"o"<<' '<<i<<' '<<j<<' '<<P[i+1][j]<< std::endl;
                    
                     
                }
             }
        
           
             outfile <<"e"<< std::endl;
             
             outfile.close();
         }
         
        // else std::cout << "Unable to open output file .vlp";
        
        
    
    }
    
   //  else std::cout << "Unable to open input file .dat or input file .txt";
    
    
        
    return parti_intere;
}




std::vector<double> build_vlp_new( int rows, int cols, int bv, int NB, int NP, int NP2,double z1int, double z2int, double M, int *bvar, double *c,double **B1, double **B2, double **P, double *b, std::string outputFile)

    {
    std::vector<double> parti_intere;
    parti_intere.clear();
    
    
    std::ofstream outfile(outputFile);
    
    

        //Building the updated b using the values of the binary variables in bvar
        double bnew[rows+1];
        double bapp;
        
        for(int i=1;i<=rows;i++)
            bnew[i]=0;
        
        for (int i=1;i<=rows;i++)
        {
            bapp=0;
            for (int j=1; j<=*&bv;j++)
            bapp=bapp+*&B2[i][j]**&bvar[j];
            bnew[i]=*&b[i]-bapp;
        }
        
       // std::cout<<"\nVettore termini noti aggiornati";
        //for(int i=1;i<=rows;i++)
          //  std::cout<<"\n"<<bnew[i];
        
   
    //Computing the integer component of the 2nd objective function
    z2int=0;
    for(int j=1; j<=bv;j++)
    {
        z2int=z2int+bvar[j]*P[4][j];
    }
    
    //Computing the integer component of the 1st objective function
    z1int=0;
    for(int j=1; j<=bv;j++)
    {
        z1int=z1int+bvar[j]*P[2][j];
    }
    
   // std::cout<<"\n Parte intera di z1:"<<z1int;
    
   // std::cout<<"\n Parte intera di z2:"<<z2int;
    
    parti_intere.push_back(z1int);
    parti_intere.push_back(z2int);
    
        
        int numObj=2;
    
    

        
        if (outfile.is_open()){
             
             outfile << "p vlp min" <<' '<<rows+1<<' '<<cols<<' '<<NB+NP2<<' '<<numObj<<' '<<NP<< std::endl;
             
            
             for (int i=1; i<=rows;i++)
             {
                 if(*&c[i]==0)
                 outfile <<"i"<<' '<<i<<'\t'<<"s"<<' '<<*&bnew[i]<< std::endl;
                 else if(c[i]==1)
                 outfile <<"i"<<' '<<i<<'\t'<<"u"<<' '<<*&bnew[i]<< std::endl;
             }
            
             
            //Additional constraint on the value of the second objective function
            outfile <<"i"<<' '<<rows+1<<'\t'<<"u"<<' '<<M-z2int<< std::endl;
            
           
                
             for (int j=1; j<=cols;j++)
             {
                 
                    // outfile <<"j"<<' '<<j<<'\t'<<"f"<< std::endl; //variabili continue libere
                    outfile <<"j"<<' '<<j<<'\t'<<"l 0"<< std::endl; //variabili continue non negative
             }
             
             
             
             for(int i=1;i<=rows;i++)
             {
                 for(int j=1; j<=cols;j++)
                 {
                         if(*&B1[i][j]!=0)
                         outfile <<"a"<<' '<<i<<' '<<j<<' '<<*&B1[i][j]<< std::endl;
                     
                 }
                // std::cout << '\n';
               
             }
            
            
            
           
            
            for(int j=1; j<=cols;j++)
            {
                    if(*&P[3][j]!=0)
                    outfile <<"a"<<' '<<rows+1<<' '<<j<<' '<<*&P[3][j]<< std::endl;
                
            }
            
           
            
            
            
             //Writing the objective coefficients in the vlp file for two objectives
             for(int i=0;i<=2;i++)
            {
                
                 for(int j=1; j<=cols;j++)
                {

                     if((*&P[i+1][j]!=0)&&(i==0))
                         outfile <<"o"<<' '<<i+1<<' '<<j<<' '<<*&P[i+1][j]<< std::endl;
                     if((*&P[i+1][j]!=0)&&(i==2))
                        outfile <<"o"<<' '<<i<<' '<<j<<' '<<*&P[i+1][j]<< std::endl;
                    
                     
                }
             }
        
           
             outfile <<"e"<< std::endl;
             
             outfile.close();
         }
         
         else std::cout << "Unable to open output file .vlp";
    
        
    return parti_intere;
}

std::vector<double> update_vlp(double b_taglio,double v1, double v2, int rows, int cols, int bv, int NB, int NP, int NP2,double z1int, double z2int, double M, int *bvar, double *c,double **B1, double **B2, double **P, double *b, std::string outputFile, int ind)

    {
    std::vector<double> parti_intere;
    parti_intere.clear();
    
    
    std::ofstream outfile(outputFile);
    
    

        //Building the updated b using the values of the binary variables in bvar
        double bnew[rows+1];
        double bapp;
        
        for(int i=1;i<=rows;i++)
            bnew[i]=0;
        
        for (int i=1;i<=rows;i++)
        {
            bapp=0;
            for (int j=1; j<=*&bv;j++)
            bapp=bapp+*&B2[i][j]**&bvar[j];
            bnew[i]=*&b[i]-bapp;
        }
        
       // std::cout<<"\nVettore termini noti aggiornati";
       // for(int i=1;i<=rows;i++)
         //   std::cout<<"\n"<<bnew[i];
        
    
    int cont=0;
    for(int j=1; j<=cols;j++)
    {
            if(*&P[1][j]*v1+*&P[3][j]*v2!=0)
            cont++;
        
    }
  //  std::cout<<"\n nonzero-coefficient counter="<<cont;
        
        int numObj=2;
    
    

        
        if (outfile.is_open()){
             
             outfile << "p vlp min" <<' '<<rows+2<<' '<<cols<<' '<<NB+NP2+cont<<' '<<numObj<<' '<<NP<< std::endl;
             
            
             for (int i=1; i<=rows;i++)
             {
                 if(*&c[i]==0)
                 outfile <<"i"<<' '<<i<<'\t'<<"s"<<' '<<*&bnew[i]<< std::endl;
                 else if(c[i]==1)
                 outfile <<"i"<<' '<<i<<'\t'<<"u"<<' '<<*&bnew[i]<< std::endl;
             }
            
             
            //Additional constraint on the value of the second objective function
            outfile <<"i"<<' '<<rows+1<<'\t'<<"u"<<' '<<M-z2int<< std::endl;
            
           
            //Additional cut constraints
            outfile <<"i"<<' '<<rows+2<<'\t'<<"u"<<' '<<b_taglio<< std::endl;
                
             for (int j=1; j<=cols;j++)
             {
                 
                    // outfile <<"j"<<' '<<j<<'\t'<<"f"<< std::endl; //variabili continue libere
                    outfile <<"j"<<' '<<j<<'\t'<<"l 0"<< std::endl; //variabili continue non negative
             }
             
             
             
             for(int i=1;i<=rows;i++)
             {
                 for(int j=1; j<=cols;j++)
                 {
                         if(*&B1[i][j]!=0)
                         outfile <<"a"<<' '<<i<<' '<<j<<' '<<*&B1[i][j]<< std::endl;
                     
                 }
                // std::cout << '\n';
               
             }
            
            
            
           
            
            for(int j=1; j<=cols;j++)
            {
                    if(*&P[3][j]!=0)
                    outfile <<"a"<<' '<<rows+1<<' '<<j<<' '<<*&P[3][j]<< std::endl;
                
            }
            
            
            //Se stiamo scrivendo uno dei tagli generati da coppie consecutive di punti estremi trovati all'iterazione precedente
            if(ind==0)
            {
                for(int j=1; j<=cols;j++)
                {
                    if((*&P[1][j]*v1+*&P[3][j]*v2)!=0)
                        outfile <<"a"<<' '<<rows+2<<' '<<j<<' '<<*&P[1][j]*v1+*&P[3][j]*v2<< std::endl;
                    
                }
                
            }
            
            //If we are writing the last cut on f2 generated by the last extreme point found in the previous iteration
            else if(ind==1)
            {
                for(int j=1; j<=cols;j++)
                {
                    if(*&P[3][j]!=0)
                        outfile <<"a"<<' '<<rows+2<<' '<<j<<' '<<*&P[3][j]<< std::endl;
                    
                }
            }
            
             //Writing the objective coefficients in the vlp file for two objectives
             for(int i=0;i<=2;i++)
            {
                
                 for(int j=1; j<=cols;j++)
                {

                     if((*&P[i+1][j]!=0)&&(i==0))
                         outfile <<"o"<<' '<<i+1<<' '<<j<<' '<<*&P[i+1][j]<< std::endl;
                     if((*&P[i+1][j]!=0)&&(i==2))
                        outfile <<"o"<<' '<<i<<' '<<j<<' '<<*&P[i+1][j]<< std::endl;
                    
                     
                }
             }
        
           
             outfile <<"e"<< std::endl;
             
             outfile.close();
         }
         
       //  else std::cout << "Unable to open output file .vlp";
    
        
    return parti_intere;
}



void remove_file(std::string s)
{
    std::ostringstream ostr;
    ostr << "rm "<< s <<"\n";
    std::string p = ostr.str();


    std::stringstream stream;
    // NOTE: this function is legacy/unused now that BENSOLVE has been
    // replaced by the in-process dichotomic-search solve (see
    // BolpDichotomic.h). If you do need it, point this at your own
    // working directory:
    stream << "cd path/to/your/instances/directory \n";
    stream << p;
    system(stream.str().c_str());
}


void bensolve(std::string s)
{
    
    std::ostringstream ostr;
    
    //no verbose
    ostr << "./bensolve "<< s <<" -s -m 0\n";
    //verbose
    //ostr << "./bensolve "<< s <<" -s \n";
    std::string p = ostr.str();
    
    
    
    // NOTE: this function is legacy/unused now that BENSOLVE has been
    // replaced by the in-process dichotomic-search solve (see
    // BolpDichotomic.h). If you do need it, install BENSOLVE and point
    // this at its directory:
    std::stringstream stream;
    stream <<"cd .. \n";
    stream <<"cd .. \n";
    stream << "cd path/to/your/bensolve/installation \n";
    stream << p;
    system(stream.str().c_str());
    
}



void define_list_extreme_solutions(std::list<std::vector<double>> &ListExtremeSol, std::string s2,   std::string s3, std::vector<double> parti_intere)
{
    
    
   
    std::fstream file2(s2);
    std::fstream file3(s3);
    std::list<std::vector<double>>::iterator itr;
    std::vector<double> app;
    app.clear();
    double a, b, c;
    
    //Definition of the list holding the integer-variable list and its corresponding list of non-dominated extreme points
    
    if (file3.is_open()&&(file2.is_open()))
    {
       /* while(file2>>a)
        app.push_back(a);
        ListExtremeSol.push_back(app);*/
        app.clear();
        a=0;
        
        while(file3>>a>>b>>c)
        {
            
            
            if(a==1)
            {
                app.push_back(b+*parti_intere.begin());
                app.push_back(c+*std::next(parti_intere.begin()));
                //Check whether the point already exists in the list, and add it only if it doesn't
                ListExtremeSol.push_back(app);
                app.clear();
                
            }
            
            
        }
        
        ListExtremeSol.sort();
        //Clean the list of any duplicates
       /* for(itr=ListExtremeSol.begin();itr!=std::prev(ListExtremeSol.end());itr++)
        {
            if(*std::next(itr)==*itr)
               {
                ListExtremeSol.erase(itr);
                itr--;
            }
            
        }*/
        
        itr=ListExtremeSol.begin();
        while(itr!=std::prev(ListExtremeSol.end()))
        {
            if(*std::next(itr)==*itr)
               {
                ListExtremeSol.erase(std::next(itr));
                itr--;
            }
            itr++;
        }
        
    }
    
    else std::cout << "Unable to open file ";
    
   // std::cout << "Number of extreme solutions \n "<<'\n';
   // std::cout << ' ' << ListExtremeSol.size()-1<<"\n";
    
    
    
    
    
  
    
    return;
}


void define_list_extreme_solutions_k(std::list<std::vector<double>> &ListExtremeSolk, std::string sk, std::vector<double> parti_intere)
{
    
    std::list<std::vector<double>>::iterator itr;
    std::fstream filek(sk);
    std::vector<double> app;
    app.clear();
    double a, b, c;
    
    //Definition of the list holding the integer-variable list and its corresponding list of non-dominated extreme points
    
    if (filek.is_open())
    {
       /* while(file2>>a)
        app.push_back(a);
        ListExtremeSol.push_back(app);*/
        app.clear();
        a=0;
        
        while(filek>>a>>b>>c)
        {
            
            
            if(a==1)
            {
                app.push_back(b+*parti_intere.begin());
                app.push_back(c+*std::next(parti_intere.begin()));
                ListExtremeSolk.push_back(app);
                app.clear();
                
            }
            
            
        }
        
        ListExtremeSolk.sort();
        
        
        //Clean the list of any duplicates
       /* for(itr=ListExtremeSolk.begin();itr!=std::prev(ListExtremeSolk.end());itr++)
        {
            if(*std::next(itr)==*itr)
               {
                ListExtremeSolk.erase(itr);
                itr--;
            }
            
        }*/
        itr=ListExtremeSolk.begin();
        while(itr!=std::prev(ListExtremeSolk.end()))
        {
            if(*std::next(itr)==*itr)
               {
                ListExtremeSolk.erase(std::next(itr));
                itr--;
            }
            itr++;
        }
        
    }
    
    else std::cout << "Unable to open file per creazione lista";
    
   // std::cout << "Number of extreme solutions \n "<<'\n';
   // std::cout << ' ' << ListExtremeSol.size()-1<<"\n";
    
    
    
    
    return;
}


void filter_list(std::list<std::vector<double>> &ListExtremeSolk, std::list<std::vector<double>> ListExtremeSol)
{
    std::list<std::vector<double>>::iterator itr, itr1;
    
    itr1=ListExtremeSol.begin();
    
    for(itr=ListExtremeSolk.begin();itr!=ListExtremeSolk.end();itr++)
           {
               
               
               if(*itr->begin()<*itr1->begin())
                  
               ListExtremeSolk.erase(itr--);

               
              
           }
    
    return ;
}





//Function to print a list of lists of double
void print_list_vector(std::list<std::vector<double>> List)
{
    
    std::list<std::vector<double>>::iterator itr;
    std::vector<double>::iterator v;
    
    
    for(itr=List.begin();itr!=List.end();++itr)
    {
        for(v=itr->begin();v!=itr->end();++v)
            std::cout<<' '<<*v;
        
        std::cout<<"\n";
    }
    return;
}

void print_time_on_txt(FILE *file, double time, int cont, int bolp)
{
    
    fprintf(file, "%lf sec\n",time);
    fprintf(file, "%d solved MILPs\n",cont);
    fprintf(file, "%d solved BOLPs\n",bolp);
}


// ============================================================================
// Tree-related functions (node, insert2, print_inorder_on_txt, etc. below):
// implementation of the binary-tree-variant data structure for storing the
// nondominated subset of a set of points/line segments in R^2, as described
// in:
//
//   N. Adelgren, P. Belotti, A. Gupte, "Efficient Storage of Pareto Points
//   in Biobjective Mixed Integer Programming," INFORMS Journal on
//   Computing, 30(2): 324-338, 2018. (arXiv:1411.6538 [cs.DS])
//
// Used here to maintain the running set of non-dominated extreme points
// discovered across BOLP slices during the branch-and-bound search.
// ============================================================================


struct node
{
    int type;
    int subtree_size;
    double slope;
    double nw_x;
    double nw_y;
    double se_x;
    double se_y;
    struct node *parent;
    struct node *left;
    struct node *right;
};

typedef struct node node;

extern node *tree;

node *find_right_leaf(node *leaf);
node *find_left_leaf(node *leaf);
node *find_rightmost_leaf(node *leaf);
node *find_leftmost_leaf(node *leaf);
node *find_deepest_leaf_right(node *leaf);

void insert(int type, double nw_x, double nw_y, double se_x, double se_y, double slope, struct node **leaf, struct node **leaf2);

void print_inorder(node *n1, int color);



#define BILLION  1000000000L;

FILE *file;        // Uncomment these lines if you wish to output MATLAB syntax
static FILE *drawing;

double x_ideal = 0.; //point reflecting about
double y_ideal = 0.;

double delta = 0.3; // Used as a rebalance parameter
int printing = 0;

int counter = 0;

double x_intersect;
double y_intersect;

/*node *tree = NULL;*/
node *tree2 = NULL;

node *find_right_leaf(node *leaf);
node *find_left_leaf(node *leaf);
node *find_deepest_leaf_right(node *leaf);
void insert2(int type, double nw_x, double nw_y, double se_x, double se_y, double slope, struct node **leaf, struct node **leaf2);
void scan_proper_tree(node *leaf);


int min(int x, int y)
{
  return ((x < y) ? x : y);
}

int max(int x, int y)
{
  return ((x > y) ? x : y);
}

//Note: this used to be the point where compilation issues started

void update_depth(node *leaf)    // This function can be used to update the depths and subtree sizes associated with each node within a certain path of the tree whenever a new node is added or a node has been removed.
{
/*    printf("leaf: %lf,%lf to %lf,%lf\n",leaf->nw_x,leaf->nw_y,leaf->se_x,leaf->se_y);*/
/*    printf("tree: %lf,%lf to %lf,%lf\n",tree->nw_x,tree->nw_y,tree->se_x,tree->se_y);*/
    if(!leaf) return;
    if(leaf->right)
    {
        if(leaf->left)
        {
            leaf->subtree_size = leaf->left->subtree_size + leaf->right->subtree_size + 1;
        }
        else
        {
            leaf->subtree_size = leaf->right->subtree_size + 1;
        }
    }
    else if(leaf->left)
    {
        leaf->subtree_size = leaf->left->subtree_size + 1;
    }
    else
    {
        leaf->subtree_size = 1;
    }
    if(leaf && leaf != tree && leaf->parent)// && leaf->parent)
    {
/*        if(printing)*/
/*        {*/
/*            printf("leaf size: %d\n",leaf->subtree_size);*/
/*            printf("tree size: %d\n",tree->subtree_size);*/
/*        }*/
        update_depth(leaf->parent);
    }
}

void print_preorder(node *n1, FILE *file)
{
        if (n1)
        {
        if(n1->type == 2)
        {
#ifdef DRAW_GRAPH
            fprintf(file,"#m=1,S=2\n%lf %lf\n%lf %lf\n\n",
                n1->nw_x,
                n1->nw_y,
                n1->se_x,
                n1->se_y);
#else
            fprintf(file,"%d: (%lf,%lf) to (%lf,%lf)\n",
                n1->type,
                n1->nw_x,
                n1->nw_y,
                n1->se_x,
                n1->se_y);
#endif
            // Uncomment the following lines in order to print MATLAB syntax for plotting the Pareto optimal solutions
                  //fprintf(file,"plot([%.24lf,%.24lf],[%.24lf,%.24lf],'-ro');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);
                  //printf("plot([%lf,%lf],[%lf,%lf],'-ro');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);
        }
        else
        {
#ifdef DRAW_GRAPH
              fprintf(file,"#m=0,S=4\n%lf %lf\n\n",
                  n1->nw_x,
                  n1->nw_y);
#else
              fprintf(file,"%d: (%lf,%lf)\n",
                  n1->type,
                  n1->nw_x,
                  n1->nw_y);
#endif
            //Also uncomment the following line in order to print the MATLAB syntax.
            //fprintf(file,"plot(%.24lf,%.24lf,'ro');\n",n1->nw_x,n1->nw_y);
            //printf("plot(%lf,%lf,'ro');\n",n1->nw_x,n1->nw_y);
        }
        if(n1->left)
        {
            printf("left child:\n");
              print_preorder(n1->left, file);
            }
            if(n1->right)
            {
                printf("right child:\n");
              print_preorder(n1->right, file);
            }
       }
}

void print_inorder(node *n1, int color)
{
        if (n1)
        {
           if(n1->left)
        {
            //printf("left child:\n");
              print_inorder(n1->left,color);
            }
        if(n1->type == 2)
        {
                  if(color == 1) printf("plot([%lf,%lf],[%lf,%lf],'-ro');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);
                  else if(color == 2) printf("plot([%lf,%lf],[%lf,%lf],'-bo');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);
/*                  if(color == 1) printf("plot([%lf,%lf],[%lf,%lf],'-rx');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);*/
/*                  else if(color == 2) printf("plot([%lf,%lf],[%lf,%lf],'-bo');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);*/
        }
        else
        {
            if(color == 1) printf("plot(%lf,%lf,'ro');\n",n1->nw_x,n1->nw_y);
            else if(color == 2) printf("plot(%lf,%lf,'bo');\n",n1->nw_x,n1->nw_y);
/*            if(color == 1) printf("plot(%lf,%lf,'rx');\n",n1->nw_x,n1->nw_y);*/
/*            else if(color == 2) printf("plot(%lf,%lf,'bo');\n",n1->nw_x,n1->nw_y);*/
        }
            if(n1->right)
            {
                //printf("right child:\n");
              print_inorder(n1->right,color);
            }
       }
}


void print_inorder_on_txt(node *n1, int color, FILE  *file)
{
    

    
        if (n1)
        {
           if(n1->left)
        {
            //printf("left child:\n");
              print_inorder_on_txt(n1->left,color, file);
            }
        if(n1->type == 2)
        {
                  if(color == 1)
                  {
                       fprintf(file, "plot([%lf,%lf],[%lf,%lf],'-ro');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);
                  }
                     // printf("plot([%lf,%lf],[%lf,%lf],'-ro');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);
                  else if(color == 2)
                  {
                      fprintf(file, "plot([%lf,%lf],[%lf,%lf],'-bo');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);
                  }
            //printf("plot([%lf,%lf],[%lf,%lf],'-bo');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);
/*                  if(color == 1) printf("plot([%lf,%lf],[%lf,%lf],'-rx');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);*/
/*                  else if(color == 2) printf("plot([%lf,%lf],[%lf,%lf],'-bo');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);*/
        }
        else
        {
            if(color == 1)
            {
                fprintf(file, "plot(%lf,%lf,'ro');\n",n1->nw_x,n1->nw_y);
            }
            //printf("plot(%lf,%lf,'ro');\n",n1->nw_x,n1->nw_y);
            else if(color == 2)
            {
                fprintf(file, "plot(%lf,%lf,'bo');\n",n1->nw_x,n1->nw_y);
            }
                
             //printf("plot(%lf,%lf,'bo');\n",n1->nw_x,n1->nw_y);
/*            if(color == 1) printf("plot(%lf,%lf,'rx');\n",n1->nw_x,n1->nw_y);*/
/*            else if(color == 2) printf("plot(%lf,%lf,'bo');\n",n1->nw_x,n1->nw_y);*/
        }
            if(n1->right)
            {
                //printf("right child:\n");
              print_inorder_on_txt(n1->right,color, file);
            }
       }
    
}



void visit_inorder_and_create_list(node *n1, int color, std::list<std::vector<double>> ListExtremeSolk)
{
    
    std::vector<double> app;
    app.clear();
    
        if (n1)
        {
           if(n1->left)
        {
            //printf("left child:\n");
              visit_inorder_and_create_list(n1->left,color, ListExtremeSolk);
            }
        if(n1->type == 2)
        {
                  if(color == 1)
                  {
                       //fprintf(file, "plot([%lf,%lf],[%lf,%lf],'-ro');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);
                      app.push_back(n1->nw_x);
                      app.push_back(n1->nw_y);
                      ListExtremeSolk.push_back(app);
                      app.clear();
                      app.push_back(n1->se_x);
                      app.push_back(n1->se_y);
                      ListExtremeSolk.push_back(app);
                      
                      
                  }
                     // printf("plot([%lf,%lf],[%lf,%lf],'-ro');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);
                  else if(color == 2)
                  {
                      fprintf(file, "plot([%lf,%lf],[%lf,%lf],'-bo');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);
                  }
            //printf("plot([%lf,%lf],[%lf,%lf],'-bo');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);
/*                  if(color == 1) printf("plot([%lf,%lf],[%lf,%lf],'-rx');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);*/
/*                  else if(color == 2) printf("plot([%lf,%lf],[%lf,%lf],'-bo');\n",n1->nw_x,n1->se_x,n1->nw_y,n1->se_y);*/
        }
        else
        {
            if(color == 1)
            {
                //fprintf(file, "plot(%lf,%lf,'ro');\n",n1->nw_x,n1->nw_y);
                app.push_back(n1->nw_x);
                app.push_back(n1->nw_y);
                ListExtremeSolk.push_back(app);
            }
            //printf("plot(%lf,%lf,'ro');\n",n1->nw_x,n1->nw_y);
            else if(color == 2)
            {
                fprintf(file, "plot(%lf,%lf,'bo');\n",n1->nw_x,n1->nw_y);
            }
                
             //printf("plot(%lf,%lf,'bo');\n",n1->nw_x,n1->nw_y);
/*            if(color == 1) printf("plot(%lf,%lf,'rx');\n",n1->nw_x,n1->nw_y);*/
/*            else if(color == 2) printf("plot(%lf,%lf,'bo');\n",n1->nw_x,n1->nw_y);*/
        }
            if(n1->right)
            {
                //printf("right child:\n");
              visit_inorder_and_create_list(n1->right,color, ListExtremeSolk);
            }
       }
    
}




void destroy_tree(struct node *leaf)
{
      if (leaf)
      {
        if(leaf->left)
          {
                 destroy_tree(leaf->left);
             }
             if(leaf->right)
             {
                  destroy_tree(leaf->right);
              }
        free( leaf );
        // leaf = NULL; // ineffectual
    }
}




// Uncomment the following function if you wish to output MATLAB syntax that plots the structure of the tree

/*void draw_tree(node *leaf, double pos1, double pos2, double inc) // Debugging procedure that prints MATLAB commands designed to draw the tree. The commands are stored in a text file: drawn_tree.txt ... when plotting, include the commands "figure();" and "hold on;"*/
/*{*/
/*    if(leaf->type == 1)*/
/*    {*/
/*        fprintf(drawing,"plot(%lf,%lf,'o');\n",pos1,pos2);*/
/*    }*/
/*    else*/
/*    {*/
/*        fprintf(drawing,"plot(%lf,%lf,'+');\n",pos1,pos2);*/
/*    }*/
/*    if(leaf->left)*/
/*    {*/
/*        fprintf(drawing,"plot([%lf,%lf],[%lf,%lf],'-');\n",pos1,pos1-inc,pos2,pos2-1);*/
/*        draw_tree(leaf->left,pos1-inc,pos2-1,inc*0.5);*/
/*    }*/
/*    if(leaf->right)*/
/*    {*/
/*        fprintf(drawing,"plot([%lf,%lf],[%lf,%lf],'-');\n",pos1,pos1+inc,pos2,pos2-1);*/
/*        draw_tree(leaf->right,pos1+inc,pos2-1,inc*0.5);*/
/*    }*/
/*} */

void scan_proper_tree(node *leaf) // Debugging procedure that makes sure only dummy nodes have children. Note that this should only be run before or after a user-implemented insertion into the tree (not an insertion generated by a recursion).
{
/*    printf("scanning node %lf,%lf to %lf,%lf\n",leaf->nw_x,leaf->nw_y,leaf->se_x,leaf->se_y);*/
    if(leaf->parent == leaf)
    {
        printf("error: leaf's parent is itself\n");
        exit(0);
    }
    if(leaf->left == leaf || leaf->right == leaf)
    {
/*        printf("the leaf: %d: %lf,%lf to %lf,%lf\n",leaf->type,leaf->nw_x,leaf->nw_y,leaf->se_x,leaf->se_y);*/
        printf("error: leaf's child is itself\n");
        exit(0);
    }
    if(leaf->nw_x - leaf->se_x > .00001 )
    {
        printf("3 -----------\n");
/*        print_inorder(tree,1);*/
        printf("scanning node %lf,%lf to %lf,%lf\n",leaf->nw_x,leaf->nw_y,leaf->se_x,leaf->se_y);
        exit(0);
    }
    if(leaf->parent && (leaf->parent->left != leaf && leaf->parent->right != leaf))
    {
        printf("error: parent pointers are F-ed up\n");
        exit(0);
    }
    if(leaf->left)
    {
        if(leaf->left->parent != leaf)
        {
            printf("error: left child has incorrect parent pointer");
            exit(0);
        }
        else if(leaf->left == leaf->parent)
        {
/*            printf("1\n");*/
        }
        else
        {
            scan_proper_tree(leaf->left);
        }
    }
    else if(leaf->right)
    {
        if(leaf->right->parent != leaf)
        {
            printf("error: right child has incorrect parent pointer");
            exit(0);
        }
        else if(leaf->right == leaf->parent)
        {
/*            printf("2\n");*/
        }
        else
        {
            scan_proper_tree(leaf->right);
        }
    }
}

void Rebalance_Right1(struct node *leaf)
{
/*    printf("rebalance right1\n");*/
    if(leaf->parent)
    {
        int t = 0;
        if(leaf == leaf->parent->right) leaf->parent->right = leaf->left;
        else {leaf->parent->left = leaf->left; t = 1;}
        if(leaf->left->right) leaf->left = leaf->left->right;
        else leaf->left = NULL;
        if(t == 0)
        {
            leaf->parent->right->right = leaf;
            leaf->parent->right->parent = leaf->parent;
            leaf->parent = leaf->parent->right;
        }
        else
        {
            leaf->parent->left->right = leaf;
            leaf->parent->left->parent = leaf->parent;
            leaf->parent = leaf->parent->left;
        }
        if(leaf->left) leaf->left->parent = leaf;
    }
    else
    {
        tree = leaf->left;
        leaf->parent = leaf->left;
        if(leaf->left->right) leaf->left = leaf->left->right;
        else leaf->left = NULL;
        leaf->parent->parent = NULL;
        leaf->parent->right = leaf;
        if(leaf->left) leaf->left->parent = leaf;
    }
    if(leaf->left) update_depth(leaf->left);
    else if(leaf->right) update_depth(leaf->right);
    else update_depth(leaf);
}

void Rebalance_Left1(struct node *leaf)
{
/*    printf("rebalance left1\n");*/
    if(leaf->parent)
    {
        int t = 0;
        if(leaf == leaf->parent->left) leaf->parent->left = leaf->right;
        else {leaf->parent->right = leaf->right; t = 1;}
        if(leaf->right->left) leaf->right = leaf->right->left;
        else leaf->right = NULL;
        if(t == 0)
        {
            leaf->parent->left->left = leaf;
            leaf->parent->left->parent = leaf->parent;
            leaf->parent = leaf->parent->left;
        }
        else
        {
            leaf->parent->right->left = leaf;
            leaf->parent->right->parent = leaf->parent;
            leaf->parent = leaf->parent->right;
        }
        if(leaf->right) leaf->right->parent = leaf;
    }
    else
    {
        tree = leaf->right;
        leaf->parent = leaf->right;
        if(leaf->right->left) leaf->right = leaf->right->left;
        else leaf->right = NULL;
        leaf->parent->parent = NULL;
        leaf->parent->left = leaf;
        if(leaf->right) leaf->right->parent = leaf;
    }
    if(leaf->left) update_depth(leaf->left);
    else if(leaf->right) update_depth(leaf->right);
    else update_depth(leaf);
}

node *find_rightmost_leaf(struct node *leaf);
node *find_leftmost_leaf(struct node *leaf);
void delete_node(node *leaf);

void Rebalance_Left2(struct node *leaf, struct node *leaf2)
{
/*    printf("rebalance left2\n");*/
    if(leaf->left)
    {
        node *temp_node = find_rightmost_leaf(leaf->left);
/*        printf("inserting 40\n");*/
        insert2(leaf->type,leaf->nw_x,leaf->nw_y,leaf->se_x,leaf->se_y,leaf->slope,&temp_node->right, &temp_node);
        if(temp_node->right)
        {
            temp_node->right->parent = temp_node;
            update_depth(temp_node);
        }
    }
    else
    {
/*        printf("inserting 41\n");*/
        insert2(leaf->type,leaf->nw_x,leaf->nw_y,leaf->se_x,leaf->se_y,leaf->slope,&leaf->left, &leaf);
        if(leaf->left)
        {
            leaf->left->parent = leaf;
            update_depth(leaf);
        }
    }
    leaf->type = leaf2->type;
    leaf->nw_x = leaf2->nw_x;
    leaf->nw_y = leaf2->nw_y;
    leaf->se_x = leaf2->se_x;
    leaf->se_y = leaf2->se_y;
    leaf->slope = leaf2->slope;
    delete_node(leaf2);
}

void Rebalance_Right2(struct node *leaf, struct node *leaf2)
{
/*    printf("rebalance right2\n");*/
    if(leaf->right)
    {
        node *temp_node = find_leftmost_leaf(leaf->right);
/*        printf("inserting 42\n");*/
        if(printing) print_preorder(tree,NULL);
        insert2(leaf->type,leaf->nw_x,leaf->nw_y,leaf->se_x,leaf->se_y,leaf->slope,&temp_node->left, &temp_node);
        if(temp_node->left)
        {
            temp_node->left->parent = temp_node;
            update_depth(temp_node);
        }
    }
    else
    {
/*        printf("inserting 43\n");*/
        insert2(leaf->type,leaf->nw_x,leaf->nw_y,leaf->se_x,leaf->se_y,leaf->slope,&leaf->right, &leaf);
        if(leaf->right)
        {
            leaf->right->parent = leaf;
            update_depth(leaf);
        }
    }
    leaf->type = leaf2->type;
    leaf->nw_x = leaf2->nw_x;
    leaf->nw_y = leaf2->nw_y;
    leaf->se_x = leaf2->se_x;
    leaf->se_y = leaf2->se_y;
    leaf->slope = leaf2->slope;
    delete_node(leaf2);
}

node *find_rightmost_leaf(struct node *leaf)
{
    node *temp_node = leaf;
    while(temp_node->right)
    {
        temp_node = temp_node->right;
    }
    return temp_node;
}

node *find_leftmost_leaf(struct node *leaf)
{
    node *temp_node = leaf;
    while(temp_node->left)
    {
        temp_node = temp_node->left;
    }
    return temp_node;
}

void Rebalance(struct node *leaf);
//int counter1 = 0;
//int counter2 = 0;

void Rebalance2(struct node *leaf)
{
    if(leaf->subtree_size > 2)
    {
        //if(leaf->left && leaf->left->subtree_size > (leaf->subtree_size - 1)/(2-delta))
        if(leaf->left && leaf->left->subtree_size > (leaf->subtree_size)/(2-delta))
        {    //print_preorder(leaf);
/*            if(leaf->right) printf("leaf subtree size: %d, left subtree size: %d, right subtree size: %d, cutoff value: %lf\n",leaf->subtree_size,leaf->left->subtree_size,leaf->right->subtree_size,(leaf->subtree_size - 1)/(2-delta));*/
            node *temp_node = find_rightmost_leaf(leaf->left);
            //if((leaf->left->left && leaf->left->left->subtree_size > (leaf->subtree_size - 1)/(2-delta) ) || temp_node == leaf->left)
            if((leaf->left->left && (leaf->left->left->subtree_size >= ((1-delta)*leaf->subtree_size)/(2-delta) - 1) && leaf->left->left->subtree_size <= (leaf->subtree_size)/(2-delta)) || temp_node == leaf->left)
            {    //counter1++;
                temp_node = leaf->left;
                Rebalance_Right1(leaf);
                Rebalance(temp_node);
            }
            else
            {    //counter2++;
                Rebalance_Right2(leaf,temp_node);
                Rebalance(leaf);
            }
        }
        //else if(leaf->right && leaf->right->subtree_size > (leaf->subtree_size - 1)/(2-delta))
        else if(leaf->right && leaf->right->subtree_size > (leaf->subtree_size)/(2-delta))
        {
            node *temp_node = find_leftmost_leaf(leaf->right);
            //if((leaf->right->right && leaf->right->right->subtree_size > (leaf->subtree_size - 1)/(2-delta) ) || temp_node == leaf->right)
            if((leaf->right->right && (leaf->right->right->subtree_size >= ((1-delta)*leaf->subtree_size)/(2-delta) - 1) && leaf->right->right->subtree_size <= (leaf->subtree_size)/(2-delta)) || temp_node == leaf->right)
            {    //counter1++;
                temp_node = leaf->right;
                Rebalance_Left1(leaf);
                Rebalance(temp_node);
            }
            else
            {    //counter2++;
                Rebalance_Left2(leaf,temp_node);
                Rebalance(leaf);
            }
        }
    }
}

void Rebalance(struct node *leaf)
{
    Rebalance2(leaf);
    if(leaf->left && leaf->left->subtree_size > 2) Rebalance(leaf->left);
    if(leaf->right && leaf->right->subtree_size > 2) Rebalance(leaf->right);
}

int times_happened = 0;
int del_root = 0;

void delete_node(node *leaf) // Algorithm 2
{
/*    printf("del_root: %d\n",del_root);*/
/*    printf("node to be deleted: %d: %lf,%lf to %lf,%lf\n",leaf->type,leaf->nw_x,leaf->nw_y,leaf->se_x,leaf->se_y);*/
/*    if(leaf)*/
/*    {*/

/*    if(!(leaf->left) && !(leaf->right))*/
/*    {*/
/*        printf("node to be deleted: %d: %lf,%lf to %lf,%lf\n",leaf->type,leaf->nw_x,leaf->nw_y,leaf->se_x,leaf->se_y);*/
/*        printf("no left or right. subtree size: %d\n",leaf->subtree_size);*/
/*        print_inorder(leaf,1);*/
/*    }*/

    if(leaf->left && (!leaf->right || (leaf->right && (leaf->left->subtree_size > leaf->right->subtree_size))))  // Algorithm 2 -- Line 4 (and 6-7)
    {
/*        printf("replacing with a left node\n");*/
        node *temp_node2 = find_rightmost_leaf(leaf->left);
        leaf->type = temp_node2->type;
        leaf->nw_x = temp_node2->nw_x;
        leaf->nw_y = temp_node2->nw_y;
        leaf->se_x = temp_node2->se_x;
        leaf->se_y = temp_node2->se_y;
        leaf->slope = temp_node2->slope;
        delete_node(temp_node2);
    }
    else if(leaf->right) // Algorithm 2 -- Line 5 (and 6-7)
    {
/*        printf("replacing with a right node\n");*/
        node *temp_node2 = find_leftmost_leaf(leaf->right);
        leaf->type = temp_node2->type;
        leaf->nw_x = temp_node2->nw_x;
        leaf->nw_y = temp_node2->nw_y;
        leaf->se_x = temp_node2->se_x;
        leaf->se_y = temp_node2->se_y;
        leaf->slope = temp_node2->slope;
        delete_node(temp_node2);
    }
    else    // Algorithm 2 Line 2 is dealt with here
    {
/*        printf("else\n");*/
        node *temp_node2 = leaf->parent;
        int t = 0;
        if(!temp_node2) t = -1;
        if(leaf == tree || leaf == tree2) t = 1;
        if(leaf->parent && leaf->parent->left && leaf == leaf->parent->left) t = 2;
        else if(leaf->parent && leaf == leaf->parent->right) t = 3;
        free(leaf);
        leaf = NULL;
        if(t == 2) temp_node2->left = NULL;
        else if(t == 3) temp_node2->right = NULL;
        if(temp_node2) update_depth(temp_node2);
    }
/*    }*/
}

node *prev_node = NULL;

void clean_it(node *nd1, int dir) // This function can be run after the Pareto set has been generated in order to combine adjacent line segments that have a non-empty intersection and the same slope (to within an epsilon tolerance).
{
    node *temp = NULL;
    if(dir != 0) temp = nd1->parent;
    if(nd1 && nd1->left)
    {
        clean_it(nd1->left,1);
    }
    if(nd1 && prev_node && nd1 != prev_node)
    {
        double xdiff = 1000.;
        double ydiff = 1000.;
        double slopediff = fabs(nd1->slope - prev_node->slope);
/*        printf("node1: %d (%lf,%lf),(%lf,%lf)\tprev_node: %d (%lf,%lf),(%lf,%lf)\n",nd1->type,nd1->nw_x,nd1->nw_y,nd1->se_x,nd1->se_y,prev_node->type,prev_node->nw_x,prev_node->nw_y,prev_node->se_x,prev_node->se_y);*/
        xdiff = nd1->nw_x - prev_node->se_x;
        ydiff = prev_node->se_y - nd1->nw_y;
        if(prev_node->type == 1)
        {
            if( xdiff < .01 && ydiff < .01 )
            {
                if( xdiff < .005 || ydiff < .005 )
                {
                    nd1->type = 2;
                    nd1->nw_x = prev_node->nw_x;
                    nd1->nw_y = prev_node->nw_y;
                    nd1->slope = (nd1->se_y-prev_node->nw_y)/(nd1->se_x-prev_node->nw_x);
                    //freed_node = prev_node;
                    delete_node(prev_node);
                }
            }
        }
        else if(prev_node->type == 2)
        {
            if( slopediff < .01 && xdiff < .01 && ydiff < .01 )
            {
                if( xdiff < .005 || ydiff < .005 )
                {
                    nd1->type = 2;
                    nd1->nw_x = prev_node->nw_x;
                    nd1->nw_y = prev_node->nw_y;
                    nd1->slope = (nd1->se_y-prev_node->nw_y)/(nd1->se_x-prev_node->nw_x);
                    //freed_node = prev_node;
                    delete_node(prev_node);
                }
            }
        }
    }
    if( dir == 0 ) nd1 = tree;
    else if( dir == 1 ) nd1 = temp->left;
    else if( dir == 2 ) nd1 = temp-> right;
    prev_node = nd1;
    if(nd1)
    {
        if(nd1->right) clean_it(nd1->right,2);
    }
}

int insert_counter = 0;
int insert_counter2 = 0;
int rebalance_count = 100;
int rebuild_count = 0;
int another_counter = 0;
/*int del_root = 0;*/

int printing_ = 0, real_counter = 0;
int insert_to_potential_branch_tree = 0;
int points_only = 0;
int its_been_only_points = 0;

/*void insert(int type, double nw_x, double nw_y, double se_x, double se_y, double slope, struct node **leaf, struct node **leaf2)*/
/*{*/
/*    start_insert_timer = clock();*/
/*    if( clock_gettime( CLOCK_REALTIME, &start1) == -1 ) {*/
/*              perror( "clock gettime" );*/
/*              exit( EXIT_FAILURE );*/
/*            }*/
/*    insert2(type, nw_x, nw_y, se_x, se_y, slope, &(*leaf), &(*leaf2));*/
/*    finish_insert_timer = clock();*/
/*    if( clock_gettime( CLOCK_REALTIME, &stop1) == -1 ) {*/
/*              perror( "clock gettime" );*/
/*              exit( EXIT_FAILURE );*/
/*            }*/

/*            accum = ( stop1.tv_sec - start1.tv_sec )*/
/*              + ( stop1.tv_nsec - start1.tv_nsec )*/
/*                / BILLION;*/
/*             insert_time2 += accum;*/
/*        insert_time += (double)(finish_struct_timer - start_struct_timer) / CLOCKS_PER_SEC;*/
/*} */

void insert2(int type, double nw_x, double nw_y, double se_x, double se_y, double slope, struct node **leaf, struct node **leaf2) // This is the main function, takes an input point or segment and removes portions of the tree that are dominated and then inserts portions of the point/segment that are not dominated into the appropriate places within the tree.
{
    if(printing)
    {
        printf("inserted data is: %d\t %.12lf,%.12lf - %.12lf,%.12lf\n",type,nw_x,nw_y,se_x,se_y);
        printf("or maybe it was: %d\t %.12lf,%.12lf - %.12lf,%.12lf\n",type,nw_x,nw_y,se_x,se_y);
    
        if(*leaf){
            printf("comparing against: %d\t %.12lf,%.12lf - %.12lf,%.12lf\n",(*leaf)->type,(*leaf)->nw_x,(*leaf)->nw_y,(*leaf)->se_x,(*leaf)->se_y);
            printf("or ...: %d\t %.12lf,%.12lf - %.12lf,%.12lf\n",(*leaf)->type,(*leaf)->nw_x,(*leaf)->nw_y,(*leaf)->se_x,(*leaf)->se_y);
        }
    }

    if((*leaf) == tree || insert_to_potential_branch_tree == 1)
    {
        if(printing) printf("here\n");
        insert_to_potential_branch_tree = 0;
        if(type == 1) // Ensures points are dealt with appropriately
        {
            se_x = nw_x;
            se_y = nw_y;
        }
/*        if(del_root == 0)*/
/*        {    */
/*            nw_x =   nw_x;*/
/*            se_x =   se_x;*/
/*            nw_y =   nw_y;*/
/*            se_y =   se_y;*/
/*            insert_counter++;*/
/*            real_counter++;*/
/*        }*/
        else del_root = 0;
        if(type == 2) slope = (se_y-nw_y)/(se_x-nw_x);
    }
    
    if(nw_x > se_x || se_y > nw_y)
    {
        if(printing) printf("dumping\n");
        return;            // Algorithm 1 -- Line 1
    }
    else if(type == 2 && fabs((double) nw_x - (double) se_x) <= .00001)
    {
        type = 1;
        nw_y = se_y;
    }
    else if(type == 2 && fabs((double) nw_y - (double) se_y) <= .00001)
    {
        type = 1;
        se_x = nw_x;
    }
    if(*leaf && type == 1 && ((fabs(nw_x - (*leaf)->nw_x) < .000001 && fabs(nw_y - (*leaf)->nw_y) < .000001) || (fabs(nw_x - (*leaf)->se_x) < .000001 && fabs(nw_y - (*leaf)->se_y) < .000001)))
    {    //printf("point already in tree\n");
        return;
    }
    else if(*leaf && type == 2 && (fabs(nw_x - (*leaf)->nw_x) < .00001 && fabs(nw_y - (*leaf)->nw_y) < .00001 && fabs(se_x - (*leaf)->se_x) < .00001 && fabs(se_y - (*leaf)->se_y) < .00001))
    {
/*        printf("segment already in tree\n");*/
        return;
    }
    if(*leaf && *leaf == tree)    // Determine whether or not rebalancing should be performed
    {
        if(insert_counter > rebalance_count) // && !show_progress)
        {
            rebalance_count = round(1.00733*insert_counter);    //use 1.00733 if rebalancing only using periodic full tree, use 8 if combining with traversed path rebalance
/*            printf("rebalancing tree\n");*/
            Rebalance(tree);
            another_counter++;
            prev_node = NULL;
        }
    }
//    if(*leaf && (*leaf)->subtree_size > 2)
//    {
//        Rebalance2(*leaf);
//    }
//    if(tree)                        //These lines can be used for debugging
//    {
//        scan_proper_tree(tree);
//    }
        if( *leaf == NULL) // Node where we are trying to insert does not exist. This means that a new *leaf must be created.
                   //    Algorithm 1 -- Line 2
       {
           if(printing) printf("creating new node: %d \t %.12lf,%.12lf  %.12lf,%.12lf\n",type,nw_x,nw_y,se_x,se_y);
       
           if(type == 2)
        {
            points_only = 0;
            its_been_only_points = 0;
        }
       
              *leaf = (struct node*) malloc( sizeof( struct node ) );
            (*leaf)->type = type;
        (*leaf)->nw_x = nw_x;
        (*leaf)->se_x = se_x;
        (*leaf)->nw_y = nw_y;
        (*leaf)->se_y = se_y;
        (*leaf)->slope = slope;
        (*leaf)->subtree_size = 1;
        if(*leaf == tree || !(*leaf2) )
        {
            (*leaf)->parent = NULL;
        }
        else
        {
/*            printf("setting parent to incoming node\n");*/
            (*leaf)->parent = *leaf2;
        }
            (*leaf)->left = NULL;
            (*leaf)->right = NULL;
            insert_counter++;
            if(*leaf != tree && *leaf2) update_depth(*leaf2);
    }
    else // The node being compared to does exist
         // Algorithm 1 -- Line 4
         // In the following code Algorithm 1 Lines 5-16 are dealt with on a case by case basis, i.e., there is separate code for:
         //        (1) An inserted singleton compared with a stored singleton
         //        (2) An inserted singleton compared with a stored segment
         //        (3) An inserted segment compared with a stored singleton
         //        (4) An inserted segment compared with a stored segment
    {
        if(type == 1) //inserted singleton
        {
            if((*leaf)->type == 1) //stored singleton
            {
                if(nw_x <= (*leaf)->nw_x && nw_y <= (*leaf)->se_y) // The point is dominated
                {    node *temp_node = NULL;
                    if((*leaf)->left) temp_node = (*leaf)->left;
                    else if((*leaf)->right) temp_node = (*leaf)->right;
                    if((*leaf)->subtree_size == 1)
                    {
                        (*leaf)->type = type;
                        (*leaf)->nw_x = nw_x;
                        (*leaf)->se_x = se_x;
                        (*leaf)->nw_y = nw_y;
                        (*leaf)->se_y = se_y;
                        (*leaf)->slope = slope;
                        insert_counter++;
                        return;
                    }
                    else
                    {
                        if(*leaf == tree) del_root = 1;
/*                        printf("(%d) deleting root\n",__LINE__);*/
                        delete_node(*leaf);
                        if(printing_) printf("inserting 1\n");
                        insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf),&(*leaf));
                    }
                }
                else if(nw_x < (*leaf)->nw_x && nw_y >= (*leaf)->se_y) // The point needs inserted left
                {
                    if(printing_) printf("inserting 2\n");
                    insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf)->left,&(*leaf));
/*                    if((*leaf)->left)*/
/*                    {*/
/*                        (*leaf)->left->parent = *leaf;*/
/*                        if((*leaf)->left->subtree_size == 1) update_depth(*leaf);*/
/*                    }*/
                }
                else if(nw_x >= (*leaf)->nw_x && nw_y < (*leaf)->se_y) // The point needs inserted right
                {
                    if(printing_) printf("inserting 3\n");
                    insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf)->right,&(*leaf));
/*                    if((*leaf)->right)*/
/*                    {*/
/*                        (*leaf)->right->parent = *leaf;*/
/*                        if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                    }*/
                }
            }
            else //stored segment
            {
                if(nw_x <= (*leaf)->nw_x && nw_y >= (*leaf)->nw_y) // The point needs inserted left
                {
                    if(printing_) printf("inserting 4\n");
                    insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf)->left,&(*leaf));
/*                    if((*leaf)->left)*/
/*                    {*/
/*                        (*leaf)->left->parent = *leaf;*/
/*                        if((*leaf)->left->subtree_size == 1) update_depth(*leaf);*/
/*                    }*/
                }
                else if(nw_x >= (*leaf)->se_x && nw_y <= (*leaf)->se_y) // The point needs inserted right
                {
                    if(printing_) printf("inserting 5\n");
                    insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf)->right,&(*leaf));
/*                    if((*leaf)->right)*/
/*                    {*/
/*                        (*leaf)->right->parent = *leaf;*/
/*                        if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                    }*/
                }
                else
                {
                    double x_proj = (*leaf)->se_x + (1./(*leaf)->slope)*(nw_y - (*leaf)->se_y);
                    double y_proj = (*leaf)->se_y + (*leaf)->slope*(nw_x - (*leaf)->se_x);
                    //printf("incoming: %lf,%lf to %lf,%lf\t current: %lf,%lf to %lf,%lf\t projection:%lf,%lf\n",nw_x,nw_y,se_x,se_y,(*leaf)->nw_x,(*leaf)->nw_y,(*leaf)->se_x,(*leaf)->se_y,x_proj,y_proj);
                    if(nw_x < x_proj || nw_y < y_proj) // The point is under (the extension of) the segment
                    {
                        if( nw_x <= (*leaf)->nw_x && x_proj >= (*leaf)->se_x ) // The entire segment is dominated
                        {    node *temp_node = NULL;
                            if((*leaf)->left) temp_node = (*leaf)->left;
                            else if((*leaf)->right) temp_node = (*leaf)->right;
                            if((*leaf)->subtree_size == 1)
                            {
                                (*leaf)->type = type;
                                (*leaf)->nw_x = nw_x;
                                (*leaf)->se_x = se_x;
                                (*leaf)->nw_y = nw_y;
                                (*leaf)->se_y = se_y;
                                (*leaf)->slope = slope;
                                insert_counter++;
                                return;
                            }
                            else
                            {
                                if(*leaf == tree) del_root = 1;
/*                                printf("(%d) deleting root\n",__LINE__);*/
                                delete_node(*leaf);
                                if(printing_) printf("inserting 6\n");
                                insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf),&(*leaf));
                            }
                        }
                        else if( nw_x <= (*leaf)->nw_x ) // The left portion of the segment is dominated
                        {
                            (*leaf)->nw_x = x_proj;
                            (*leaf)->nw_y = nw_y;
                            if(printing_) printf("inserting 7\n");
                            insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf)->left,&(*leaf));
/*                            if((*leaf)->left)*/
/*                            {*/
/*                                (*leaf)->left->parent = *leaf;*/
/*                                if((*leaf)->left->subtree_size == 1) update_depth(*leaf);*/
/*                            }*/
                        }
                        else if( x_proj >= (*leaf)->se_x ) // The right portion of the segment is dominated
                        {
                            (*leaf)->se_y = (*leaf)->se_y + ((*leaf)->se_y - (*leaf)->nw_y)/((*leaf)->se_x - (*leaf)->nw_x)*(nw_x - (*leaf)->se_x);
                            (*leaf)->se_x = nw_x;
                            if(printing_) printf("inserting 8\n");
                            insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf)->right,&(*leaf));
/*                            if((*leaf)->right)*/
/*                            {*/
/*                                (*leaf)->right->parent = *leaf;*/
/*                                if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                            }*/
                        }
                        else // The center portion of the segment is dominated
                        {
                            double save1 = (*leaf)->se_x;
                            double save2 = (*leaf)->se_y;
                            (*leaf)->se_y = (*leaf)->se_y + (*leaf)->slope*(nw_x - (*leaf)->se_x);
                            (*leaf)->se_x = nw_x;
                            if(printing_) printf("inserting 8b\n");
                            insert2((*leaf)->type,x_proj,nw_y,save1,save2,(*leaf)->slope,&(*leaf)->right,&(*leaf));
/*                            if((*leaf)->right)*/
/*                            {    */
/*                                (*leaf)->right->parent = *leaf;*/
/*                                if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                            }*/
                            if(printing_) printf("inserting 9\n");
                            insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf)->right,&(*leaf));
/*                            if((*leaf)->right)*/
/*                            {*/
/*                                (*leaf)->right->parent = *leaf;*/
/*                                if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                            }*/
                        }
                    }
                }
            }
        }
        else // Input is a line segment
        {
            if((*leaf)->type == 1) // Incoming segment compared against current point
            {
                double y_proj = se_y + slope*((*leaf)->nw_x - se_x);
                double x_proj = se_x + (1./slope)*((*leaf)->nw_y - se_y);
                //printf("incoming: %lf,%lf to %lf,%lf\t current: %lf,%lf to %lf,%lf\n",nw_x,nw_y,se_x,se_y,(*leaf)->nw_x,(*leaf)->nw_y,(*leaf)->se_x,(*leaf)->se_y);
                //printf("the projections are: %.12lf,%.12lf and %.12lf,%.12lf\n",x_proj,(*leaf)->nw_y,(*leaf)->nw_x,y_proj);
                //printf("slope: %lf\t calculated: %lf\n",slope,(se_y - nw_y)/(se_x - nw_x));
                if(se_x <= (*leaf)->nw_x && se_y >= (*leaf)->nw_y) // The segment needs inserted left
                {    if(printing_) printf("inserting 10\n");
                    insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf)->left,&(*leaf));
/*                    if((*leaf)->left)*/
/*                    {*/
/*                        (*leaf)->left->parent = *leaf;*/
/*                        if((*leaf)->left->subtree_size == 1) update_depth(*leaf);*/
/*                    }*/
                }
                else if(nw_x >= (*leaf)->nw_x && nw_y <= (*leaf)->nw_y) // The segment needs inserted right
                {
                    if(printing_) printf("inserting 11\n");
                    insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf)->right,&(*leaf));
/*                    if((*leaf)->right)*/
/*                    {*/
/*                        (*leaf)->right->parent = *leaf;*/
/*                        if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                    }*/
                }
                else if(x_proj <= ((*leaf)->nw_x + .0001) || y_proj <= ((*leaf)->nw_y + .0001)) // The point is dominated
                {    node *temp_node = NULL;
                    if((*leaf)->left) temp_node = (*leaf)->left;
                    else if((*leaf)->right) temp_node = (*leaf)->right;
                    if((*leaf)->subtree_size == 1)
                    {
                        (*leaf)->type = type;
                        (*leaf)->nw_x = nw_x;
                        (*leaf)->se_x = se_x;
                        (*leaf)->nw_y = nw_y;
                        (*leaf)->se_y = se_y;
                        (*leaf)->slope = slope;
                        insert_counter++;
                        return;
                    }
                    else
                    {
                        if(*leaf == tree) del_root = 1;
/*                        printf("(%d) deleting root\n",__LINE__);*/
                        delete_node(*leaf);
                        if(printing_) printf("inserting 12\n");
                        insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf),&(*leaf));
                    }
                }
                else
                {
                    double save = (*leaf)->nw_y;
                    if(printing_) printf("inserting 13\n");
                    insert2(type,nw_x,nw_y,(*leaf)->nw_x,y_proj,slope,&(*leaf)->left,&(*leaf));
/*                    if((*leaf)->left)*/
/*                    {*/
/*                        (*leaf)->left->parent = *leaf;*/
/*                        if((*leaf)->left->subtree_size == 1) update_depth(*leaf);*/
/*                    }*/
                    if(printing_) printf("inserting 14\n");
                    insert2(type,x_proj,save,se_x,se_y,slope,&(*leaf)->right,&(*leaf));
/*                    if((*leaf)->right)*/
/*                    {*/
/*                        (*leaf)->right->parent = *leaf;*/
/*                        if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                    }*/
                }
            }
            else // Incoming segment compared against current segment
            {
                if(se_x <= (*leaf)->nw_x && se_y >= (*leaf)->nw_y) // The segment needs inserted left
                {
                    if(printing_) printf("inserting 15\n");
                    insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf)->left,&(*leaf));
/*                    if((*leaf)->left)*/
/*                    {*/
/*                        (*leaf)->left->parent = *leaf;*/
/*                        if((*leaf)->left->subtree_size == 1) update_depth(*leaf);*/
/*                    }*/
                }
                else if(nw_x >= (*leaf)->se_x && nw_y <= (*leaf)->se_y) // The segment needs inserted right
                {    //printf("incoming segment: %lf,%lf to %lf,%lf \t current segment: %lf,%lf to %lf,%lf\n",nw_x,nw_y,se_x,se_y,(*leaf)->nw_x,(*leaf)->nw_y,(*leaf)->se_x,(*leaf)->se_y);
                    if(printing_) printf("inserting 16\n");
                    insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf)->right,&(*leaf));
/*                    if((*leaf)->right)*/
/*                    {*/
/*                        (*leaf)->right->parent = *leaf;*/
/*                        if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                    }*/
                }
                else
                {
/*                    printf("incoming segment: %.12lf,%.12lf to %.12lf,%.12lf \t current segment: %.12lf,%.12lf to %.12lf,%.12lf\n",nw_x,nw_y,se_x,se_y,(*leaf)->nw_x,(*leaf)->nw_y,(*leaf)->se_x,(*leaf)->se_y);*/
/*                    double x_intersect = ((*leaf)->nw_y*se_x*(*leaf)->se_x-(*leaf)->nw_y*nw_x*(*leaf)->se_x-nw_y*se_x*(*leaf)->se_x+nw_y*se_x*(*leaf)->nw_x+se_y*nw_x*(*leaf)->se_x-se_y*nw_x*(*leaf)->nw_x-(*leaf)->se_y*se_x*(*leaf)->nw_x+(*leaf)->se_y*nw_x*(*leaf)->nw_x)/(-(*leaf)->se_y*se_x+(*leaf)->se_y*nw_x+(*leaf)->nw_y*se_x-(*leaf)->nw_y*nw_x+se_y*(*leaf)->se_x-se_y*(*leaf)->nw_x-nw_y*(*leaf)->se_x+nw_y*(*leaf)->nw_x); // Find the intersection of (the possible extensions of) the line segments being compared*/
                    double x_intersect = (slope*se_x-(*leaf)->slope*(*leaf)->se_x-se_y+(*leaf)->se_y)/(-(*leaf)->slope+slope);
                    double y_intersect = (*leaf)->nw_y + (*leaf)->slope*(x_intersect - (*leaf)->nw_x);
                    if(y_intersect != y_intersect) y_intersect = nw_y + slope*(x_intersect - nw_x);
                    if(printing) printf("the intersection is: %.12lf,%.12lf\n",x_intersect,y_intersect);
                    if(printing) printf("plot(%lf,%lf,'go');\n",x_intersect,y_intersect);
                    double nw_y_proj_c2i = se_y + slope*((*leaf)->nw_x - se_x); // projection of the endpoints of current segment onto incoming segment (hence, c2i)
                    double se_y_proj_c2i = se_y + slope*((*leaf)->se_x - se_x);
                    double nw_x_proj_c2i = se_x + (1./slope)*((*leaf)->nw_y - se_y);
                    double se_x_proj_c2i = se_x + (1./slope)*((*leaf)->se_y - se_y);
                    double nw_y_proj_i2c = (*leaf)->se_y + (*leaf)->slope*(nw_x - (*leaf)->se_x); // projection of the endpoints of incoming segment onto current segment (hence, i2c)
                    double se_y_proj_i2c = (*leaf)->se_y + (*leaf)->slope*(se_x - (*leaf)->se_x);
                    double nw_x_proj_i2c = (*leaf)->se_x + (1./(*leaf)->slope)*(nw_y - (*leaf)->se_y);
                    double se_x_proj_i2c = (*leaf)->se_x + (1./(*leaf)->slope)*(se_y - (*leaf)->se_y);
                    //printf("the projections: %.12lf,%.12lf,  %.12lf,%.12lf,  %.12lf,%.12lf,  %.12lf,%.12lf,  %.12lf,%.12lf,  %.12lf,%.12lf,  %.12lf,%.12lf,  %.12lf,%.12lf\n",(*leaf)->nw_x,nw_y_proj_c2i,(*leaf)->se_x,se_y_proj_c2i,nw_x_proj_c2i,(*leaf)->nw_y,se_x_proj_c2i,(*leaf)->se_y,nw_x,nw_y_proj_i2c,se_x,se_y_proj_i2c,nw_x_proj_i2c,nw_y,se_x_proj_i2c,se_y);
                    if( (*leaf)->nw_x - x_intersect <= .0000001 && x_intersect - (*leaf)->se_x <= .0000001 && nw_x - x_intersect <= .0000001
                        && x_intersect - se_x <= .0000001 ) // The segments intersect
                    {
                        if( (double)nw_x_proj_c2i <= (*leaf)->nw_x || (double)nw_y_proj_c2i <= (*leaf)->nw_y ) // Left side of incoming segment is below current segment
                        {
                            if( (*leaf)->nw_x < nw_x ) // Left most portion of current segment is non-dominated
                            {
                                double save1 = (*leaf)->se_y;
                                double save2 = (*leaf)->se_x;
                                    (*leaf)->se_x = nw_x;
                                (*leaf)->se_y = (double)nw_y_proj_i2c;
                                if(printing_) printf("inserting 17\n");
                                insert2((*leaf)->type,x_intersect,y_intersect,save2,save1,(*leaf)->slope,&(*leaf)->right,&(*leaf));
/*                                if((*leaf)->right)*/
/*                                {*/
/*                                    (*leaf)->right->parent = *leaf;*/
/*                                    if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                                }*/
                                if(printing_) printf("inserting 18\n");
                                insert2(type,nw_x,nw_y,x_intersect,y_intersect,slope,&(*leaf)->right,&(*leaf));
/*                                if((*leaf)->right)*/
/*                                {*/
/*                                    (*leaf)->right->parent = *leaf;*/
/*                                    if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                                }*/
                                if(se_x_proj_c2i == se_x_proj_c2i)
                                {
                                    if(printing_) printf("inserting 19\n");
                                    insert2(type,(double)se_x_proj_c2i,save1,se_x,se_y,slope,&(*leaf)->right,&(*leaf));
/*                                    if((*leaf)->right)*/
/*                                    {*/
/*                                        (*leaf)->right->parent = *leaf;*/
/*                                        if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                                    }*/
                                }
                            }
                            else
                            {
                                (*leaf)->nw_x = x_intersect;
                                (*leaf)->nw_y = y_intersect;
                                double save = (*leaf)->se_y;
                                if(printing_) printf("inserting 20\n");
                                insert2(type,nw_x,nw_y,x_intersect,y_intersect,slope,&(*leaf)->left,&(*leaf));
/*                                if((*leaf)->left)*/
/*                                {*/
/*                                    (*leaf)->left->parent = *leaf;*/
/*                                    if((*leaf)->left->subtree_size == 1) update_depth(*leaf);*/
/*                                }*/
                                if(se_x_proj_c2i == se_x_proj_c2i)
                                {
                                    if(printing_) printf("inserting 21\n");
                                    insert2(type,(double)se_x_proj_c2i,save,se_x,se_y,slope,&(*leaf)->right,&(*leaf));
/*                                    if((*leaf)->right)*/
/*                                    {*/
/*                                        (*leaf)->right->parent = *leaf;*/
/*                                        if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                                    }*/
                                }
                            }
                        }
                        else // Right side of incoming segment is below current segment
                        {    //printf("right side of incoming segment is below current segment\n");
                            if( (*leaf)->se_y < se_y ) // Right most portion of current segment is non-dominated
                            {    //printf("Right most portion of current segment is non-dominated\n");
                                double save1 = (*leaf)->se_x;
                                double save2 = (*leaf)->se_y;
                                (*leaf)->se_x = x_intersect;
                                (*leaf)->se_y = y_intersect;
                                double save = (*leaf)->nw_x;
                                if(se_x_proj_i2c == se_x_proj_i2c)
                                {
                                    if(printing_) printf("inserting 22\n");
                                    insert2((*leaf)->type,(double)se_x_proj_i2c,se_y,save1,save2,
                                        (*leaf)->slope,&(*leaf)->right,&(*leaf));
/*                                        if((*leaf)->right)*/
/*                                        {*/
/*                                            (*leaf)->right->parent = *leaf;*/
/*                                            if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                                        }*/
                                }
                                if(printing_) printf("inserting 23\n");
                                insert2(type,x_intersect,y_intersect,se_x,se_y,slope,&(*leaf)->right,&(*leaf));
/*                                    if((*leaf)->right)*/
/*                                    {*/
/*                                        (*leaf)->right->parent = *leaf;*/
/*                                        if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                                    }*/
                                if(nw_y_proj_c2i == nw_y_proj_c2i)
                                {
                                    if(printing_) printf("inserting 24\n");
                                    insert2(type,nw_x,nw_y,save,(double)nw_y_proj_c2i,slope,&(*leaf)->left,&(*leaf));
/*                                        if((*leaf)->left)*/
/*                                        {*/
/*                                            (*leaf)->left->parent = *leaf;*/
/*                                            if((*leaf)->left->subtree_size == 1) update_depth(*leaf);*/
/*                                        }*/
                                }
                            }
                            else
                            {
                                (*leaf)->se_x = x_intersect;
                                (*leaf)->se_y = y_intersect;
                                double save = (*leaf)->nw_x;
                                if(printing_) printf("inserting 25\n");
                                insert2(type,x_intersect,y_intersect,se_x,se_y,slope,&(*leaf)->right,&(*leaf));
/*                                    if((*leaf)->right)*/
/*                                    {*/
/*                                        (*leaf)->right->parent = *leaf;*/
/*                                        if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                                    }*/
                                if(nw_y_proj_c2i == nw_y_proj_c2i)
                                {
                                    if(printing_) printf("inserting 26\n");
                                    insert2(type,nw_x,nw_y,save,(double)nw_y_proj_c2i,slope,&(*leaf)->left,&(*leaf));
/*                                        if((*leaf)->left)*/
/*                                        {*/
/*                                            (*leaf)->left->parent = *leaf;*/
/*                                            if((*leaf)->left->subtree_size == 1) update_depth(*leaf);*/
/*                                        }*/
                                }
                            }
                        }
                    }
                    else // The segments don't intersect
                    {
                            if( nw_x <= (*leaf)->nw_x ) // first point of incoming segment is left of current segment
                            {
                                if((*leaf)->nw_y <= (double)nw_y_proj_c2i) // || (*leaf)->se_y <= (double)se_y_proj_c2i )
                                {
                                    double save = (*leaf)->se_y;
                                    if(nw_y_proj_c2i == nw_y_proj_c2i)
                                    {
                                        if(printing_) printf("inserting 27\n");
                                        insert2(type,nw_x,nw_y,(*leaf)->nw_x,(double)nw_y_proj_c2i,slope,
                                            &(*leaf)->left,&(*leaf));
/*                                        if((*leaf)->left)*/
/*                                        {*/
/*                                            (*leaf)->left->parent = *leaf;*/
/*                                            if((*leaf)->left->subtree_size == 1) update_depth(*leaf);*/
/*                                        }*/
                                    }
                                    if(se_x_proj_c2i == se_x_proj_c2i)
                                    {
                                        if(printing_) printf("inserting 28\n");
                                        insert2(type,(double)se_x_proj_c2i,save,se_x,se_y,slope,&(*leaf)->right,&(*leaf));
/*                                        if((*leaf)->right)*/
/*                                        {*/
/*                                            (*leaf)->right->parent = *leaf;*/
/*                                            if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                                        }*/
                                    }
                                }
                                else
                                {
                                    if( se_y <= (*leaf)->se_y ) // incoming segment dominates current segment
                                    {    node *temp_node = NULL;
                                        if((*leaf)->left) temp_node = (*leaf)->left;
                                        else if((*leaf)->right) temp_node = (*leaf)->right;
                                        else temp_node = (*leaf)->parent;
                                        if((*leaf)->subtree_size == 1)
                                        {
                                            (*leaf)->type = type;
                                            (*leaf)->nw_x = nw_x;
                                            (*leaf)->se_x = se_x;
                                            (*leaf)->nw_y = nw_y;
                                            (*leaf)->se_y = se_y;
                                            (*leaf)->slope = slope;
                                            insert_counter++;
                                            return;
                                        }
                                        else
                                        {
                                            if(*leaf == tree) del_root = 1;
/*                                            printf("(%d) deleting root\n",__LINE__);*/
                                            delete_node(*leaf);
                                            if(printing_) printf("inserting 29\n");
                                            insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf),&(*leaf));
                                        }
                                    }
                                    else // upper portion of current segment dominated
                                    {
                                        (*leaf)->nw_x = (double)se_x_proj_i2c;
                                        (*leaf)->nw_y = se_y;
                                        if(printing_) printf("inserting 30\n");
                                        insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf)->left,&(*leaf));
/*                                        if((*leaf)->left)*/
/*                                        {*/
/*                                            (*leaf)->left->parent = *leaf;*/
/*                                            if((*leaf)->left->subtree_size == 1) update_depth(*leaf);*/
/*                                        }*/
                                    }
                                }
                            }
                            else if( nw_y <= (double)nw_y_proj_i2c ) // first point of incoming segment lies below the current segment
                            {
                                if( se_y <= (*leaf)->se_y ) // lower portion of current segment dominated
                                {
                                    (*leaf)->se_x = nw_x;
                                    (*leaf)->se_y = (double)nw_y_proj_i2c;
                                    if(printing_) printf("inserting 31\n");
                                    insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf)->right,&(*leaf));
/*                                    if((*leaf)->right)*/
/*                                    {*/
/*                                        (*leaf)->right->parent = *leaf;*/
/*                                        if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                                    }*/
                                }
                                else // center portion of current segment dominated
                                {
                                    double save1 = (*leaf)->se_x;
                                    double save2 = (*leaf)->se_y;
                                        (*leaf)->se_x = nw_x;
                                    (*leaf)->se_y = (double)nw_y_proj_i2c;
                                    if(se_x_proj_i2c == se_x_proj_i2c)
                                    {
                                        if(printing_) printf("inserting 31.5\n");
                                        insert2((*leaf)->type,(double)se_x_proj_i2c,se_y,save1,save2,(*leaf)->slope,
                                            &(*leaf)->right,&(*leaf));
/*                                        if((*leaf)->right)*/
/*                                        {*/
/*                                            (*leaf)->right->parent = *leaf;*/
/*                                            if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                                        }*/
                                    }
                                    if(printing_) printf("inserting 32\n");
                                    insert2(type,nw_x,nw_y,se_x,se_y,slope,&(*leaf)->right,&(*leaf));
/*                                    if((*leaf)->right)*/
/*                                    {*/
/*                                        (*leaf)->right->parent = *leaf;*/
/*                                        if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                                    }*/
                                }
                            }
                            else // first point of incoming segment lies above or to the right of current segment
                            {
                                if(se_x_proj_c2i == se_x_proj_c2i)
                                {
                                    if(printing_) printf("inserting 33\n");
                                    insert2(type,(double)se_x_proj_c2i,(*leaf)->se_y,se_x,se_y,slope,&(*leaf)->right,&(*leaf));
/*                                    if((*leaf)->right)*/
/*                                    {*/
/*                                        (*leaf)->right->parent = *leaf;*/
/*                                        if((*leaf)->right->subtree_size == 1) update_depth(*leaf);*/
/*                                    }*/
                                }
                            }
                        }
                    }
                }
            }
        }
}

int get_tree_depth(node *leaf)
{
    if(leaf->left && leaf->right) return max(get_tree_depth(leaf->left) + 1,get_tree_depth(leaf->right) + 1);
    else if(leaf->left) return get_tree_depth(leaf->left) + 1;
    else if(leaf->right) return get_tree_depth(leaf->right) + 1;
    else return 0;
}

int get_num_nodes(node *leaf)
{
    if(leaf->left && leaf->right) return get_num_nodes(leaf->left)+get_num_nodes(leaf->right)+1;
    else if(leaf->left) return get_num_nodes(leaf->left) + 1;
    else if(leaf->right) return get_num_nodes(leaf->right) + 1;
    else return 1;
}

int get_num_inserts()
{
    return real_counter;
}

/*int get_num_mock_inserts()*/
/*{*/
/*    return mock_insert_counter;*/
/*}*/




#endif /* Header_h */




