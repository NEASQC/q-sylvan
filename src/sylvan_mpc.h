/*
 * Copyright 2023 System Verification Lab, LIACS, Leiden University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 */

/**
 * This is an implementation of the Multi Precision Complex numbers library 
 * MPC for custom leaves of the MTBDDs.
 * 
 * The source code was copied from "sylvan_gmp.h/c" initially.
 * 
 */

#include <sylvan.h>
#include <mpc.h>

#ifndef SYLVAN_MPC_H
#define SYLVAN_MPC_H

#ifdef __cplusplus
namespace sylvan {
extern "C" {
#endif /* __cplusplus */

// Base of the representation of the float number in the complex number
#define MPC_BASE_OF_FLOAT 10

// Rounding the real and imaginary numbers to zero = ZZ 
#define MPC_ROUNDING MPC_RNDZZ

// Number of bits for the complex number
#define MPC_PRECISION 256

// Max buffer length to handle filestrings
#define MPC_MAXLENGTH_FILESTRING 256

// TODO: Global still needed?
static uint32_t mpc_type;

/**
 * Initialize MPC custom leaves
 */
uint32_t 
mpc_init(void);

/**
 * Create MPC leaf
 */
MTBDD 
mtbdd_mpc(mpc_t val);

/**
 * Assign a MPC complex number
*/
void 
mpc_assign(mpc_ptr complexnumber, double real, double imag);

/**
 * Compare MPC variables
*/
int 
mpc_compare(const uint64_t left, const uint64_t right);

/**
 * Operation "plus" for two mpc MTBDDs
 */
TASK_DECL_2(MTBDD, mpc_op_plus, MTBDD*, MTBDD*);

//TASK_DECL_3(MTBDD, gmp_abstract_op_plus, MTBDD, MTBDD, int);

/**
 * Operation "minus" for two mpq MTBDDs
 */
//TASK_DECL_2(MTBDD, gmp_op_minus, MTBDD*, MTBDD*);

/**
 * Operation "times" for two mpq MTBDDs
 */
TASK_DECL_2(MTBDD, mpc_op_times, MTBDD*, MTBDD*);
//TASK_DECL_3(MTBDD, gmp_abstract_op_times, MTBDD, MTBDD, int);

/**
 * Operation "divide" for two mpq MTBDDs
 */
//TASK_DECL_2(MTBDD, gmp_op_divide, MTBDD*, MTBDD*);

/**
 * Operation "min" for two mpq MTBDDs
 */
//TASK_DECL_2(MTBDD, gmp_op_min, MTBDD*, MTBDD*);
//TASK_DECL_3(MTBDD, gmp_abstract_op_min, MTBDD, MTBDD, int);

/**
 * Operation "max" for two mpq MTBDDs
 */
//TASK_DECL_2(MTBDD, gmp_op_max, MTBDD*, MTBDD*);
//TASK_DECL_3(MTBDD, gmp_abstract_op_max, MTBDD, MTBDD, int);

/**
 * Operation "negate" for one mpq MTBDD
 */
//TASK_DECL_2(MTBDD, gmp_op_neg, MTBDD, size_t);

/**
 * Operation "abs" for one mpq MTBDD
 */
//TASK_DECL_2(MTBDD, gmp_op_abs, MTBDD, size_t);

/**
 * Compute a + b
 */
#define mpc_plus(a, b) mtbdd_apply(a, b, TASK(mpc_op_plus))

/**
 * Compute a + b
 */
//#define gmp_minus(a, b) mtbdd_apply(a, b, TASK(gmp_op_minus))

/**
 * Compute a * b
 */
//#define gmp_times(a, b) mtbdd_apply(a, b, TASK(gmp_op_times))

/**
 * Compute a * b
 */
//#define gmp_divide(a, b) mtbdd_apply(a, b, TASK(gmp_op_divide))

/**
 * Compute min(a, b)
 */
//#define gmp_min(a, b) mtbdd_apply(a, b, TASK(gmp_op_min))

/**
 * Compute max(a, b)
 */
//#define gmp_max(a, b) mtbdd_apply(a, b, TASK(gmp_op_max))

/**
 * Compute -a
 */
//#define gmp_neg(a) mtbdd_uapply(a, TASK(gmp_op_neg), 0);

/**
 * Compute abs(a)
 */
//#define gmp_abs(a) mtbdd_uapply(a, TASK(gmp_op_abs), 0);

// TODO: abstract == reduce

/**
 * Abstract the variables in <v> from <a> by taking the sum of all values
 */
//#define gmp_abstract_plus(dd, v) mtbdd_abstract(dd, v, TASK(gmp_abstract_op_plus))

/**
 * Abstract the variables in <v> from <a> by taking the product of all values
 */
//#define gmp_abstract_times(dd, v) mtbdd_abstract(dd, v, TASK(gmp_abstract_op_times))

/**
 * Abstract the variables in <v> from <a> by taking the minimum of all values
 */
//#define gmp_abstract_min(dd, v) mtbdd_abstract(dd, v, TASK(gmp_abstract_op_min))

/**
 * Abstract the variables in <v> from <a> by taking the maximum of all values
 */
//#define gmp_abstract_max(dd, v) mtbdd_abstract(dd, v, TASK(gmp_abstract_op_max))

/**
 * Multiply <a> and <b>, and abstract variables <vars> using summation.
 * This is similar to the "and_exists" operation in BDDs.
 */
//TASK_DECL_3(MTBDD, gmp_and_abstract_plus, MTBDD, MTBDD, MTBDD);
//#define gmp_and_abstract_plus(a, b, vars) RUN(gmp_and_abstract_plus, a, b, vars)
//#define gmp_and_exists gmp_and_abstract_plus

/**
 * Multiply <a> and <b>, and abstract variables <vars> by taking the maximum.
 */
//TASK_DECL_3(MTBDD, gmp_and_abstract_max, MTBDD, MTBDD, MTBDD);
//#define gmp_and_abstract_max(a, b, vars) RUN(gmp_and_abstract_max, a, b, vars)

/**
 * Convert to a Boolean MTBDD, translate terminals >= value to 1 and to 0 otherwise;
 * Parameter <dd> is the MTBDD to convert; parameter <value> is an GMP mpq leaf
 */
//TASK_DECL_2(MTBDD, gmp_op_threshold, MTBDD*, MTBDD*);
//#define gmp_threshold(dd, value) mtbdd_apply(dd, value, TASK(gmp_op_threshold));

// TODO: strict = restrict?

/**
 * Convert to a Boolean MTBDD, translate terminals > value to 1 and to 0 otherwise;
 * Parameter <dd> is the MTBDD to convert; parameter <value> is an GMP mpq leaf
 */
//TASK_DECL_2(MTBDD, gmp_op_strict_threshold, MTBDD*, MTBDD*);
//#define gmp_strict_threshold(dd, value) mtbdd_apply(dd, value, TASK(gmp_op_strict_threshold));

/**
 * Convert to a Boolean MTBDD, translate terminals >= value to 1 and to 0 otherwise;
 */
//TASK_DECL_2(MTBDD, gmp_threshold_d, MTBDD, double);
//#define gmp_threshold_d(dd, value) RUN(gmp_threshold_d, dd, value)

/**
 * Convert to a Boolean MTBDD, translate terminals > value to 1 and to 0 otherwise;
 */
//TASK_DECL_2(MTBDD, gmp_strict_threshold_d, MTBDD, double);
//#define gmp_strict_threshold_d(dd, value) RUN(gmp_strict_threshold_d, dd, value)

#ifdef __cplusplus
}
}
#endif /* __cplusplus */

#endif
