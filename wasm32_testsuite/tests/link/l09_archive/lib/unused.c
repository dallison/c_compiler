// Nothing asks for this, so it must never reach the link.  If it did, the
// call below would fail to resolve and the link would fail, which is what
// makes leaving it out something the test can actually see.
int nothing_defines_this(void);

int unused_entry(void) { return nothing_defines_this(); }
