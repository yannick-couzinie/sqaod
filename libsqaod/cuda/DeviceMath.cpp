#include "DeviceMath.h"

using namespace sqaod_cuda;

template<class real>
void DeviceMathType<real>::setToDiagonals(DeviceMatrix *A, real v) {
    size_t size = std::min(A->rows, A->cols);
    devCopy_(A, v, size, A->rows + 1, 0);
}

template<class real>
void DeviceMathType<real>::scale(DeviceScalar *y, real alpha, const DeviceScalar &x,
                                 real addAssignFactor) {
    scale(y->d_data, alpha, x.d_data, addAssignFactor);
}

template<class real>
void DeviceMathType<real>::scale(DeviceVector *y, real alpha, const DeviceVector &x,
                                 real addAssignFactor) {
    THROW_IF(y->size != x.size, "Vector length does not match.");
    scale(y->d_data, alpha, x.d_data, addAssignFactor);
}

template<class real>
void DeviceMathType<real>::scaleBroadcast(DeviceVector *y, real alpha, const DeviceScalar &x,
                                          real addAssignFactor) {
    scaleBroadcast(y->d_data, alpha, x.d_data, y->size, addAssignFactor);
}

template<class real>
void DeviceMathType<real>::scaleBroadcast(DeviceMatrix *A, real alpha, const DeviceVector &x,
                                          BatchOp op, real addAssignFactor) {
    if (op == opRowwise) {
        THROW_IF(A->cols != x.size, "Cols of matrix does not match vector length.");
        scaleBroadcastVector(A->d_data, alpha, x.d_data, x.size, A->cols, addAssignFactor);
    }
    else if (op == opColwise) {
        THROW_IF(A->rows != x.size, "Rows of matrix does not match vector length.");
        scaleBroadcastScalars(A->d_data, alpha, x.d_data, x.size, A->cols, addAssignFactor);
    }
    else {
        THROW("Unknown matrix op.");
    }
}
    
template<class real>
void DeviceMathType<real>::sum(DeviceScalar *s, real alpha, const DeviceVector &x,
                               real addAssignFactor) {
    sum(s->d_data, alpha, x.d_data, x.size, addAssignFactor);
}

template<class real>
void DeviceMathType<real>::sum(DeviceScalar *s, real alpha, const DeviceMatrix &dmat,
                               real addAssignFactor) {
    sum(s->d_data, alpha, dmat.d_data, dmat.rows * dmat.cols, addAssignFactor);
}

template<class real>
void DeviceMathType<real>::sumDiagonals(DeviceScalar *s, const DeviceMatrix &dmat) {
    int nElms = std::min(dmat.rows, dmat.cols);
    sumGather(s->d_data, 1., dmat.d_data, nElms, dmat.cols + 1, 0);
}

template<class real>
void DeviceMathType<real>::sumBatched(DeviceVector *vec,
                                      real alpha, const DeviceMatrix &A, BatchOp op) {
    const DeviceMatrix *dmat;
    if (op == opColwise) {
        DeviceMatrix *transposed = tempDeviceMatrix(A.dim());
        transpose(transposed, A);
        dmat = transposed;
    }
    else {
        assert(op == opRowwise);
        dmat = &A;
    }
    sumBatched(vec->d_data, 1., dmat->d_data, dmat->cols, dmat->rows);
}

template<class real>
void DeviceMathType<real>::dot(DeviceScalar *z,
                               real alpha, const DeviceVector &x, const DeviceVector &y,
                               real addAssignFactor) {
    THROW_IF(x.size != y.size, "Vector length does not match.");
    dot(z->d_data, alpha, x.d_data, y.d_data, x.size, addAssignFactor);
}

template<class real>
void DeviceMathType<real>::dotBatched(DeviceVector *z, real alpha,
                                      const DeviceMatrix &A, MatrixOp opA,
                                      const DeviceMatrix &B, MatrixOp opB, real addAssignFactor) {
    const DeviceMatrix *dMat0, *dMat1;
    if (opA == opTranspose) {
        DeviceMatrix *dAt;
        dAt = tempDeviceMatrix(A.rows, A.cols);
        transpose(dAt, A);
        dMat0 = dAt;
    }
    else {
        assert(opA == opNone);
        dMat0 = &A;
    }
    if (opB == opTranspose) {
        DeviceMatrix *dBt;
        dBt = tempDeviceMatrix(B.rows, B.cols);
        transpose(dBt, A);
        dMat0 = dBt;
    }
    else {
        assert(opB == opNone);
        dMat1 = &B;
    }
    dotBatched(z->d_data, alpha, dMat0->d_data, dMat1->d_data, dMat0->cols, dMat0->rows,
               addAssignFactor);
}

template<class real>
void DeviceMathType<real>::mvProduct(DeviceVector *y, real alpha,
                                     const DeviceMatrix &A, MatrixOp opA, const DeviceVector &x) {
    const DeviceScalar &d_alpha = deviceConst(alpha);
    gemv(opA, d_alpha, A, x, d_zero(), *y);
}

template<class real>
void DeviceMathType<real>::vmProduct(DeviceVector *y, real alpha,
                                     const DeviceVector &x, const DeviceMatrix &A, MatrixOp opA,
                                     real addAssignFactor) {
    const DeviceScalar &d_alpha = deviceConst(alpha);
    const DeviceScalar &d_factor = deviceConst(addAssignFactor);
    opA = (opA == opNone) ? opTranspose : opNone;
    gemv(opA, d_alpha, A, x, d_factor, *y);
}

template<class real>
void DeviceMathType<real>::mmProduct(DeviceMatrix *C, real alpha,
                                     const DeviceMatrix &A, MatrixOp opA,
                                     const DeviceMatrix &B, MatrixOp opB) {
    const DeviceScalar &d_alpha = deviceConst(alpha);
    gemm(opA, opB, d_alpha, A, B, d_zero(), *C);
}
    
template<class real>
void DeviceMathType<real>::vmvProduct(DeviceScalar *z, real alpha,
                                      const DeviceVector &y, const DeviceMatrix &A,
                                      const DeviceVector &x) {
    DeviceVector *Ax = tempDeviceVector(A.rows);
    gemv(opNone, d_one(), A, x, d_zero(), *Ax);
    dot(z, 1., y, *Ax);
}

template<class real>
void DeviceMathType<real>::batchedVmvProduct(DeviceVector *z, real alpha,
                                             const DeviceMatrix &y,
                                             const DeviceMatrix &A,
                                             const DeviceMatrix &x) {
    sqaod::Dim dim = getProductShape(x, opNone, A, opTranspose);
    DeviceMatrix *Ax = tempDeviceMatrix(dim);
    gemm(opTranspose, opNone, d_one(), x, A, d_zero(), *Ax);
    dotBatched(z, alpha, *Ax, opNone, x, opNone);
}

template<class real>
void DeviceMathType<real>::mmmProduct(DeviceMatrix *z, real alpha,
                                      const DeviceMatrix &y, MatrixOp opy,
                                      const DeviceMatrix &A, MatrixOp opA,
                                      const DeviceMatrix &x, MatrixOp opx) {
    const DeviceScalar &d_alpha = deviceConst(alpha);
    
    sqaod::Dim dimAx = getProductShape(A, opA, x, opx);
    DeviceMatrix *Ax =  tempDeviceMatrix(dimAx);
    gemm(opA, opx, d_one(), A, x, d_zero(), *Ax);
    gemm(opy, opNone, d_alpha, y, *Ax, d_zero(), *z);
}

template<class real>
void DeviceMathType<real>::min(DeviceScalar *s, const DeviceMatrix &A) {
    
}


template struct sqaod_cuda::DeviceMathType<float>;
template struct sqaod_cuda::DeviceMathType<double>;

