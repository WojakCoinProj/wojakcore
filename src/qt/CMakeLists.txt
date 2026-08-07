# Qt resources
qt5_add_resources(QT_RESOURCES
    bitcoin.qrc
)

# UI files
qt5_wrap_ui(UI_HEADERS
    forms/aboutdialog.ui
    forms/addressbookpage.ui
    forms/askpassphrasedialog.ui
    forms/coincontroldialog.ui
    forms/editaddressdialog.ui
    forms/intro.ui
    forms/openuridialog.ui
    forms/optionsdialog.ui
    forms/overviewpage.ui
    forms/qrcodedialog.ui
    forms/receivecoinsdialog.ui
    forms/receiverequestdialog.ui
    forms/rpcconsole.ui
    forms/sendcoinsdialog.ui
    forms/sendcoinsentry.ui
    forms/signverifymessagedialog.ui
    forms/transactiondescdialog.ui
)

# MOC headers
qt5_wrap_cpp(MOC_SOURCES
    aboutdialog.h
    addressbookpage.h
    addresstablemodel.h
    askpassphrasedialog.h
    bitcoinaddressvalidator.h
    bitcoingui.h
    bitcoinunits.h
    clientmodel.h
    coincontroldialog.h
    coincontroltreewidget.h
    csvmodelwriter.h
    editaddressdialog.h
    guiutil.h
    intro.h
    macdockiconhandler.h
    macnotificationhandler.h
    monitoreddatamapper.h
    notificator.h
    optionsdialog.h
    optionsmodel.h
    overviewpage.h
    paymentserver.h
    qrcodedialog.h
    qvalidatedlineedit.h
    qvaluecombobox.h
    rpcconsole.h
    sendcoinsdialog.h
    sendcoinsentry.h
    signverifymessagedialog.h
    transactiondescdialog.h
    transactionfilterproxy.h
    transactionrecord.h
    transactiontablemodel.h
    transactionview.h
    walletframe.h
    walletmodel.h
    walletstack.h
    walletview.h
)

# Source files
set(QT_SOURCES
    aboutdialog.cpp
    addressbookpage.cpp
    addresstablemodel.cpp
    askpassphrasedialog.cpp
    bitcoin.cpp
    bitcoinaddressvalidator.cpp
    bitcoinamountfield.cpp
    bitcoingui.cpp
    bitcoinstrings.cpp
    bitcoinunits.cpp
    clientmodel.cpp
    coincontroldialog.cpp
    coincontroltreewidget.cpp
    csvmodelwriter.cpp
    editaddressdialog.cpp
    guiutil.cpp
    intro.cpp
    macdockiconhandler.mm
    macnotificationhandler.mm
    monitoreddatamapper.cpp
    notificator.cpp
    optionsdialog.cpp
    optionsmodel.cpp
    overviewpage.cpp
    paymentserver.cpp
    qrcodedialog.cpp
    qvalidatedlineedit.cpp
    qvaluecombobox.cpp
    rpcconsole.cpp
    sendcoinsdialog.cpp
    sendcoinsentry.cpp
    signverifymessagedialog.cpp
    splashscreen.cpp
    transactiondesc.cpp
    transactiondescdialog.cpp
    transactionfilterproxy.cpp
    transactionrecord.cpp
    transactiontablemodel.cpp
    transactionview.cpp
    walletframe.cpp
    walletmodel.cpp
    walletstack.cpp
    walletview.cpp
)

# Add the executable
add_executable(wojakcoin-qt
    ${QT_SOURCES}
    ${MOC_SOURCES}
    ${UI_HEADERS}
    ${QT_RESOURCES}
)

# Link libraries
target_link_libraries(wojakcoin-qt
    PRIVATE
    wojakcoin_core
    leveldb
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::DBus
    ${OPENSSL_LIBRARIES}
    ${BDB_LIBRARIES}
)

# Set properties
set_target_properties(wojakcoin-qt PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# Install the application
install(TARGETS wojakcoin-qt
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
