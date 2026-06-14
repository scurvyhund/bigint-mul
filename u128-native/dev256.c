/* dev256.c on local git branch dev256.      10/23/25 
 *
 * Program prints the product of two (spc seperated) unsigned decimal strings 
 * given by user as cmdln_args. Args can represent values 0 to (2^128)-1.
 *
 * Example run with output:
 *
 * $> ./dev256 123456789 987654321
 *
 * Product: 121932631112635269
 *
 *
 * Formula to build 256 bit result:
 *
 *  -----  high 128 bits  ------      --- low 128 bits ---
 * (((hihi << 64) + hilo) << 128)  +  (lohi << 64) + lolo
 *
 * hihi: 0, hilo: 0, lohi: 0, lolo: 121932631112635269
 * 
 *
 * build with: gcc -gdwarf-5 -Wall -Wextra -std=c99 -m64 -o exec source
 */


#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <ctype.h>
#include <inttypes.h>

typedef unsigned long long u64;
typedef __uint128_t u128;
typedef struct u256 u256;

// Function prototypes
static int validate_cmdln_args(char* a);
static int string_compare(const char* a , const char* b);
static u128 string_to_u128(const char* a);
void mul256b(u256* , u256* , u256* );
char* u256_to_string(u256* );

struct u256 {
   u64 lo;
   u64 mid;
   u128 hi;
};

#define MAX_128   "340282366920938463463374607431768211455"

static int validate_cmdln_args(char* cmdln_arg) {

   if (cmdln_arg[0] == '\0') {
      fprintf(stderr, "\nError: Empty input string.\n\n");
      return 1;
   }
   for (size_t i = 0; cmdln_arg[i]; i++) {
      if (!isdigit((unsigned char)cmdln_arg[i])) {
         fprintf(stderr, "\nError: Invalid character '%c' in input\n\n",\
                 cmdln_arg[i]);
         return 1;
      }
   }
   return 0;
}

static int string_compare(const char *a, const char *b) {
    // Skip leading zeros
    while (*a == '0' && *(a + 1) != '\0') a++;
    while (*b == '0' && *(b + 1) != '\0') b++;

    size_t len_a = strlen(a);
    size_t len_b = strlen(b);

    // Different lengths = different magnitudes
  if (len_a != len_b) {
    return (len_a < len_b) ? -1 : 1;
  }

  // Same length, compare digit by digit
  return strcmp(a, b);
}

static u128 string_to_u128(const char *str) {
   
   u128 result = 0;
   for (size_t i = 0; str[i]; i++) {
      result = result * 10 + (str[i] - '0');
   }

   return result;
}

void mul256b(u256 *x, u256 *y, u256 *tmp_struc_ptr) {

   /* Local variables assigned values calced with ptr (x, and y) to main()
    * structs binum1 and bignum2.
    */
   u128 t1 = (u128)x->lo * y->lo;
   u128 t2 = (u128)x->lo * y->mid;
   u128 t3 = (u128)x->mid * y->lo;
   u128 t4 = (u128)x->mid * y->mid;

   // lo 64 bits of t1 (1) -> lo.
   u64 lo = t1;

   // hi 64 bits t1 ((2^64)-2)  + low 64 bits t2.
   u128 m1 = (t1 >> 64) + (u64)t2;

   u64 m2 = m1;  
   u128 mid = (u128)m2 + (u64)t3;
    
   u128 hi = (t2 >> 64) + (t3 >> 64) + t4 + (m1 >> 64) + (mid >> 64);
   
   // assign final calc'ed partial results to tmp struct members...
   tmp_struc_ptr->lo = lo;
   tmp_struc_ptr->mid = mid;
   tmp_struc_ptr->hi = hi;
}

// Converts large integers to printable strings, arg is &tmp.
char* u256_to_string(u256* tmp_struc_ptr) {

   // Heap memory for (Max digits in (2^256)+1)...digits used like
   // an array.
   char* digits = malloc(79);
   
   // init local varialbles with u256 tmp struct member values...
   u64 lo  = tmp_struc_ptr->lo;
   u64 mid = tmp_struc_ptr->mid;
   u128 hi = tmp_struc_ptr->hi;
   
   // Product is 0 (zero) 
   if (hi == 0 && mid == 0 && lo == 0) {
      digits[0] = '0';
      digits[1] = '\0';
      return digits;
    }

   // Branchless version - replaced if block...
   // Only skips if ALL are zero...    
   size_t digit_count = 0;
   while (hi != 0 || mid != 0 || lo != 0) {
      
      u128 remainder = 0;
      remainder = hi % 10;
      hi = hi / 10;

      u128 mid_extended = (remainder << 64) + mid;
      remainder = mid_extended % 10;
      mid = mid_extended / 10;

      u128 lo_extended = (remainder << 64) + lo;
      remainder = lo_extended % 10;
      lo = lo_extended / 10;

      digits[digit_count++] = (char)(remainder + '0');
   }
   // null terminator
   digits[digit_count] = '\0';

   // load 
   for (size_t i = 0; i < digit_count / 2; i++) {
      char temp = digits[i];

      // start before null terminator...   
      digits[i] = digits[digit_count - 1 - i];
      digits[digit_count - 1 - i] = temp;
   }
   return digits;
}

#ifndef TESTING

int main(int argc, char** argv) {
   
   if (argc != 3) {
      fprintf(stderr, "\nBad argc count!!! Exiting with error code 1\n");
      fprintf(stderr, "\nUsage:\n    $> ./dev256 <num1> <num2>\n\n");
      exit(1);
   }

   // lt zero if arg[x] < MAX_128, eq if argv[x] == MAX_128 , gt zero if
   // argv[x] > MAX_128 
   if ((string_compare(argv[1], MAX_128) > 0) || (string_compare(argv[2],\
        MAX_128) > 0)) {
      puts("\nERROR: Factor[s] exceeds MAX_128 value: ((2^128)-1) \n");
      return 2;
   }

   // Check cmdln_args and assign to varialbles
   char* x_str = argv[1];
   if (validate_cmdln_args(x_str))
      return 3;

   char* y_str = argv[2];
   if (validate_cmdln_args(y_str))
      return 4;
   
   // Convert cmdln_args to u128 integers...
   u128 x_int = string_to_u128(x_str);
   u128 y_int = string_to_u128(y_str);

   // Create u256 structs and struct ptrs, zero init members 
   u256 bignum1 = {0}, bignum2 = {0};
   
   // Struct pointers...
   u256* x = &bignum1;
   u256* y = &bignum2;
   
   // and init struct members via ptrs. x and y...
   x->lo = (u64)x_int;     // low 64 bits of x_int
   x->mid = x_int >> 64;   // hi 64 bits of x_int

   y->lo = (u64)y_int;
   y->mid = y_int >> 64;

   // Pass empty struc to hold final values loded in and ret'd from mul256b.
   u256 tmp = {0};
   mul256b(x, y, &tmp);
   
   char* product_str = u256_to_string(&tmp);
   
   printf("\nProduct: %s\n", product_str);

   // Free mem alloc'd as 'digits' in u256_to_string() for 79 digit chars.
   free(product_str);
    
   // Print partial values and Formula to construct Product values...
   puts("\n\t *\n\t * Formula to build 256 bit result:\n\t *\
       \n\t *  -----  high 128 bits  ------      --- low 128 bits ---\
       \n\t * (((hihi << 64) + hilo) << 128)  +  (lohi << 64) + lolo\
       \n\t *");

   u64 hihi = tmp.hi >> 64;
   u64 hilo = (u64)tmp.hi;

   printf("\nhihi: %llu, hilo: %llu, lohi: %llu, lolo: %llu\n\n", hihi, hilo,\
         tmp.mid, tmp.lo);

   return 0;
}
#endif
