libgerbera_deps += [ gmock_dep, gtest_dep ]

subdir('config')
subdir('content')
subdir('core')
subdir('database')
if duktape_dep.found() and curl_dep.found()
  subdir('scripting')
endif
subdir('util')
