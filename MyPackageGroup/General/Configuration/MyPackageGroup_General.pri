isEmpty(MyPackageGroup_General_PRI_INCLUDED) {
  message ( loading MyPackageGroup_General.pri )
}
# **InsertLicense** code
# -----------------------------------------------------------------------------
# MyPackageGroup_General prifile
#
# \file    MyPackageGroup_General.pri
# \author  thomas
# \date    2015-10-25
#
# 
#
# -----------------------------------------------------------------------------

# include guard against multiple inclusion
isEmpty(MyPackageGroup_General_PRI_INCLUDED) {

MyPackageGroup_General_PRI_INCLUDED = 1

# -- System -------------------------------------------------------------

include( $(MLAB_MeVis_Foundation)/Configuration/SystemInit.pri )

# -- Define local PACKAGE variables -------------------------------------

PACKAGE_ROOT    = $$(MLAB_MyPackageGroup_General)
PACKAGE_SOURCES = "$$(MLAB_MyPackageGroup_General)"/Sources

# Add package library path
LIBS          += -L"$${PACKAGE_ROOT}"/lib

# -- Projects -------------------------------------------------------------

# NOTE: Add projects below to make them available to other projects via the CONFIG mechanism

# You can use this example template for typical projects:
#MLMyProject {
#  CONFIG_FOUND += MLMyProject
#  INCLUDEPATH += $${PACKAGE_SOURCES}/ML/MLMyProject
#  win32:LIBS += MLMyProject$${d}.lib
#  unix:LIBS += -lMLMyProject$${d}
#}

# -- ML Projects -------------------------------------------------------------

# -- Inventor Projects -------------------------------------------------------

# -- Shared Projects ---------------------------------------------------------

# End of projects ------------------------------------------------------------

}