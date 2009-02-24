enum {
    UNIT_LEN = 11
};

double unit_atof( const char *s );
iperf_size_t unit_atoi( const char *s );
void unit_snprintf( char *s, int inLen, double inNum, char inFormat );
