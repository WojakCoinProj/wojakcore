# Add source files for the core library
set(CORE_SOURCES
    addrman.cpp
    alert.cpp
    bitcoinrpc.cpp
    bloom.cpp
    chainparams.cpp
    checkpoints.cpp
    core.cpp
    crypter.cpp
    db.cpp
    hash.cpp
    init.cpp
    key.cpp
    keystore.cpp
    leveldb.cpp
    main.cpp
    miner.cpp
    net.cpp
    netbase.cpp
    noui.cpp
    protocol.cpp
    rpcblockchain.cpp
    rpcdump.cpp
    rpcmining.cpp
    rpcnet.cpp
    rpcrawtransaction.cpp
    rpcwallet.cpp
    script.cpp
    sync.cpp
    txdb.cpp
    util.cpp
    version.cpp
    wallet.cpp
    walletdb.cpp
)

# Add header files for the core library
set(CORE_HEADERS
    addrman.h
    alert.h
    allocators.h
    base58.h
    bignum.h
    bitcoinrpc.h
    bloom.h
    chainparams.h
    checkpoints.h
    checkqueue.h
    clientversion.h
    coincontrol.h
    compat.h
    core.h
    crypter.h
    db.h
    hash.h
    init.h
    key.h
    keystore.h
    leveldb.h
    limitedmap.h
    main.h
    miner.h
    mruset.h
    net.h
    netbase.h
    noui.h
    protocol.h
    rpcprotocol.h
    script.h
    serialize.h
    sync.h
    threadsafety.h
    tinyformat.h
    txdb.h
    ui_interface.h
    uint256.h
    util.h
    version.h
    wallet.h
    walletdb.h
)

# Add the core library
add_library(wojakcoin_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})

# Link libraries
if(USE_DEPENDS)
    target_link_libraries(wojakcoin_core
        PRIVATE
        leveldb
        ssl
        crypto
        db_cxx
        boost_system
        boost_filesystem
        boost_thread
        boost_chrono
    )
    
    # Add link directories for depends
    target_link_directories(wojakcoin_core
        PRIVATE
        ${DEPENDS_PREFIX}/lib
    )
    
    # Add compile definitions for depends
    target_compile_definitions(wojakcoin_core
        PRIVATE
        -DBOOST_FILESYSTEM_VERSION=3
        -D_FILE_OFFSET_BITS=64
        -DQT_GUI
        -DQT_NO_DEBUG
        -DHAVE_CONFIG_H
        -DSTATICLIB
    )
    
    # Add include directories for depends
    target_include_directories(wojakcoin_core
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/leveldb/include
        ${CMAKE_CURRENT_SOURCE_DIR}/leveldb/helpers
        ${DEPENDS_PREFIX}/include
    )
else()
    # System libraries configuration
    target_link_libraries(wojakcoin_core
        PRIVATE
        leveldb
        ${Boost_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${BDB_LIBRARIES}
    )
    
    target_include_directories(wojakcoin_core
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${LEVELDB_INCLUDE_DIRS}
        ${BDB_INCLUDE_DIR}
    )
    
    target_compile_definitions(wojakcoin_core
        PRIVATE
        -DBOOST_FILESYSTEM_VERSION=3
        -D_FILE_OFFSET_BITS=64
        -DQT_GUI
        -DQT_NO_DEBUG
    )
endif()

# Common compiler flags
if(MSVC)
    target_compile_options(wojakcoin_core PRIVATE /W4 /WX)
else()
    target_compile_options(wojakcoin_core PRIVATE -Wall -Wextra -Wformat -Wformat-security -Wno-unused-parameter)
    
    # Add hardening flags for release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(wojakcoin_core PRIVATE -O2 -g -fstack-protector-strong -fstack-protector-all -D_FORTIFY_SOURCE=2)
        target_link_options(wojakcoin_core PRIVATE -Wl,-z,now -Wl,-z,relro -Wl,-z,noexecstack -pie)
    endif()
endif()

# Add subdirectories
add_subdirectory(qt)
