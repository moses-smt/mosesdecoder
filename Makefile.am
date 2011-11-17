# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4

# order is important here: build moses before moses-cmd
if WITH_MERT
    MERT = mert
endif
if WITH_SERVER
    SERVER = server
endif
SUBDIRS = util lm moses/src OnDiskPt/src moses-cmd/src misc moses-chart-cmd/src CreateOnDisk/src $(MERT) $(SERVER)
