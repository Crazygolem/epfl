#!/usr/bin/Rscript

# Loading Libraries
library('plyr') # aggregation: ddply()
library('calibrate') # annotation: textxy()

# Loading data
d <- read.csv('../data/communications_rtt_4.dat', sep=' ')

# Filtering data
d <- d[d$payload_size > 0,] # True RTT (no payload) cannot tell about speed
#d <- d[d$num_messages == 1,]
d$conc_messages <- d$num_messages # Messages sent concurrently (not aggregated)
d$message_id <- NULL
d$arrival_rank <- NULL

# Aggregation
d <- ddply(d, .(payload_size, delay, conc_messages), colwise(mean))
# We cannot compute actual payload size here, as R uses
# 32-bit integers and the total payload might be too large.
#d$payload_size <- d$payload_size * d$num_messages

# Computation of the network speed
d$hrtt <- d$rtt_ms / 2
#d$netspeed <- (d$payload_size * 32) / (d$hrtt / 1000) # payload: 32-bit ints, hrtt [ms] -> [s]
d$netspeed <- (d$payload_size * 32) / (d$hrtt / 1000) * d$num_messages

# Plotting function
myplot <- function(df, outname) {
  outfile <- paste0(outname, '.svg')
  svg(outfile, width=10, height=8)

  # Scatterplot
  plot(df$payload_size, df$netspeed,
    col=as.numeric(df$delay),
    pch=as.numeric(df$delay),
    main="Network speed",
    xlab="Payload size [# of integers]",
    ylab="Network speed [b/s]")

  # Labels near points
  textxy(df$payload_size, df$netspeed, df$conc_messages,
    cex=0.8,
    offset=1.2)

  # Legend
  legend("topright",
    levels(df$delay),
    pch=as.numeric(levels(factor(as.numeric(df$delay)))),
    col=as.numeric(levels(factor(as.numeric(df$delay)))),
    title='Load management')

  dev.off()
}

# Last filtering and plotting
myplot(d, 'netspeed_vs_payload')
