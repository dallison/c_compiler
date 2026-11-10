// Chrono clocks need these hooks.  The ESP32 interpreter has no host clock
// bridge yet, so both clocks report a frozen epoch.
long long __davecc_monotonic_time_us(void) { return 0; }
long long __davecc_realtime_time_us(void) { return 0; }
