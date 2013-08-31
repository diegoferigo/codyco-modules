/* 
 * Copyright (C) 2013 CoDyCo
 * Author: Andrea Del Prete
 * email:  andrea.delprete@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

#include <locomotion/equalityQP.h>
#include <Eigen/SVD>
#include <yarp/sig/Matrix.h>
#include <iCub/ctrl/math.h>
#include <iostream>
#include <cstdio>

using namespace locomotion;
using namespace Eigen;
using namespace std;
using namespace iCub::ctrl;
using namespace yarp::math;

//*************************************************************************************************************************
yarp::sig::Vector locomotion::compute6DError(const yarp::sig::Vector &x, const yarp::sig::Vector &xd)
{
    yarp::sig::Matrix R     = axis2dcm(x.subVector(3,6));
    yarp::sig::Matrix Rdes  = axis2dcm(xd.subVector(3,6));
    yarp::sig::Matrix Re    = Rdes * R.transposed();
    yarp::sig::Vector aa    = dcm2axis(Re);
    yarp::sig::Vector res(6);
    res[0] = xd[0]-x[0];
    res[1] = xd[1]-x[1];
    res[2] = xd[2]-x[2];
    res[3] = aa[3] * aa[0];
    res[4] = aa[3] * aa[1];
    res[5] = aa[3] * aa[2];
    return res;
}

//*************************************************************************************************************************
void locomotion::pinvTrunc(const MatrixXd &A, double tol, MatrixXd &Apinv, VectorXd *svP)
{
    // allocate memory
    int m = A.rows(), n = A.cols(), k = m<n?m:n;
    MatrixXd Spinv=MatrixXd::Zero(k,k), SpinvD=MatrixXd::Zero(k,k);
    // compute decomposition
    JacobiSVD<MatrixXd> svd(A, ComputeThinU | ComputeThinV);    // default Eigen SVD
    VectorXd sv = svd.singularValues();
    // compute pseudoinverse of singular value matrix
    for (int c=0;c<k; c++)
        if ( sv(c)> tol)
            Spinv(c,c) = 1/sv(c);
    // compute pseudoinverse
    Apinv = svd.matrixV() * Spinv  * svd.matrixU().transpose();
    if(svP!=0)
        *svP = sv;
}


//*************************************************************************************************************************
void locomotion::pinvDampTrunc(const MatrixXd &A, double tol, double damp, MatrixXd &Apinv, MatrixXd &ApinvDamp, VectorXd *svP)
{
    // allocate memory
    int m = A.rows(), n = A.cols(), k = m<n?m:n;
    MatrixXd Spinv=MatrixXd::Zero(k,k), SpinvD=MatrixXd::Zero(k,k);
    // compute decomposition
    JacobiSVD<MatrixXd> svd(A, ComputeThinU | ComputeThinV);    // default Eigen SVD
    VectorXd sv = svd.singularValues();
    // compute pseudoinverses of singular value matrix
    double damp2 = damp*damp;
    for (int c=0;c<k; c++)
    {
        SpinvD(c,c) = sv(c) / (sv(c)*sv(c) + damp2);
        if ( sv(c)> tol)
            Spinv(c,c) = 1/sv(c);
    }
    // compute pseudoinverses
    Apinv       = svd.matrixV() * Spinv  * svd.matrixU().transpose();   // truncated pseudoinverse
    ApinvDamp   = svd.matrixV() * SpinvD * svd.matrixU().transpose();   // damped pseudoinverse
    if(svP!=0)
        *svP = sv;
}

//*************************************************************************************************************************
void locomotion::assertEqual(const MatrixXd &A, const MatrixXd &B, string testName, double tol)
{
    if(A.cols() != B.cols() || A.rows()!=B.rows())
    {
        cout<< testName<< ": dim(A) != dim(B): " << A.rows()<< "x"<<A.cols()<<" != "<< B.rows()<< "x"<<B.cols()<<endl;
        return testFailed(testName);
    }
    for(int r=0; r<A.rows(); r++)
        for(int c=0; c<A.cols(); c++)
            if(abs(A(r,c)-B(r,c))>tol)
            {
                printf("%s: element %d,%d is different, absolute difference is %f\n", testName.c_str(), r, c, abs(A(r,c)-B(r,c)));
                return testFailed(testName);
            }
}

//*************************************************************************************************************************
void locomotion::testFailed(string testName)
{
    printf("Test %s ***FAILED*** !!!\n", testName.c_str());
    assert(false);
}

//*************************************************************************************************************************
std::string locomotion::toString(const Eigen::MatrixXd &m, int precision, const char* endRowStr)
{
    string ret = "";
    char tmp[350];
    for(int i=0;i<m.rows();i++)
    {
        for(int j=0;j<m.cols();j++)
        {
            sprintf(tmp, "% .*lf\t", precision, m(i,j));
            ret+=tmp;
        }
        ret = ret.substr(0,ret.length()-1);     // remove the last character (tab)
        if(i<m.rows()-1)                          // if it is not the last row
            ret+= endRowStr;
    }
    return ret.substr(0, ret.length()-1);
}