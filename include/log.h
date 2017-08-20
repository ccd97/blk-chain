#ifndef __LOG_H__
#define __LOG_H__

#define LOGGING 1
#define DEBUG 1
#define ERROR 1

// log macros
#if LOGGING
  #define logging(type, x) std::cout<<type<<": "<<__FILE__<<":"<<__LINE__<<" : "<<#x<<"="<<x<<std::endl
  #define logging_arr(type, arr) std::cout<<type<<": "<<__FILE__<<":"<<__LINE__<<" : "<<#arr<<"={ ";for(int i=0; i<size_arr(arr); i++) std::cout<<arr[i]<<" "; print("}")
  #define logging_msg(type, x) std::cout<<type<<": "<<__FILE__<<":"<<__LINE__<<" : "<<x<<std::endl
#else
  #define logging(type, x) // no logging
  #define logging_arr(type, a) // no logging
  #define logging_msg(type, x) // no logging
#endif

// Debug macros
#if DEBUG
  #define debug(x) logging("Debug", x)
  #define debug_arr(arr) logging_arr("Debug", arr)
  #define dmsg(x) logging_msg("Debug", x)
#else
  #define debug(x) // no debug
  #define debug_arr(a) // no debug
  #define dmsg(x) // no debug
#endif

// Error macros
#if ERROR
  #define error(x) logging("ERROR", x)
  #define error_arr(arr) logging_arr("ERROR", arr)
  #define err(x) logging_msg("ERROR", x)
#else
  #define error(x) // no error
  #define error_arr(a) // no error
  #define err(x) // no error
#endif


#endif
