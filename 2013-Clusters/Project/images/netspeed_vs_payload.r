d <- read.csv('../data/communications_rtt_2.dat', sep=' ')
d$hrtt <- d$rtt_ms / 2

# payload of 32-bit ints, hrtt in ms -> netspeed in b/s
d$netspeed <- (d$payload_size * 32) / (d$hrtt / 1000)

d <- aggregate(d, by=list(d$payload_size, d$slaves_processing), FUN=mean)
d$slaves_processing <- factor(d$Group.2)
d$Group.2 <- NULL
d$Group.1 <- NULL

dl <- d[d$payload_size <= 10000000,]
dh <- d[d$payload_size > 10000000,]

svg('netspeed_vs_payload.svg', width=10, height=8)
x <- d
plot(x$netspeed ~ x$payload_size,
  col=as.numeric(x$slaves_processing),
  pch=as.numeric(x$slaves_processing),
  main="Network speed",
  xlab="Payload size [# of integers]",
  ylab="Network speed [b/s]")
legend("bottomright",
  levels(x$slaves_processing),
  pch=as.numeric(levels(factor(as.numeric(x$slaves_processing)))),
  col=as.numeric(levels(factor(as.numeric(x$slaves_processing)))),
  title='Load management')
dev.off()

svg('netspeed_vs_payload_l.svg', width=10, height=8)
x <- dl
plot(x$netspeed ~ x$payload_size,
  col=as.numeric(x$slaves_processing),
  pch=as.numeric(x$slaves_processing),
  main="Network speed",
  xlab="Payload size [# of integers]",
  ylab="Network speed [b/s]")
legend("bottomright",
  levels(x$slaves_processing),
  pch=as.numeric(levels(factor(as.numeric(x$slaves_processing)))),
  col=as.numeric(levels(factor(as.numeric(x$slaves_processing)))),
  title='Load management')
dev.off()
