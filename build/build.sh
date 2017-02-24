project_dir=$PWD/..
build_dir=$project_dir/build
object_dir=$project_dir/object
output_path=$project_dir/output
exe_name=main_exe
debug_make=

if [ "$1" ]; then
    debug_make="VERBOSE=1" #if you want to see gcc flags then you will use "./build.sh 1"
fi

if [ ! -d $build_dir ]; then
    echo "directory $build_dir is not exist, please clone the build directory to $build_dir."
    exit 1
fi

if [ ! -d $project_dir/code ]; then
    echo "$project_dir/code directory is not exist, please clone the code directory to  $project_dir."
    exit 2
fi

if [ ! -d $object_dir ]; then
    mkdir -p $object_dir
fi

cd $object_dir
rm -fr *
if [ -f $output_path/$exe_name ]; then
    rm $output_path/$exe_name
fi

corenum=$(cat /proc/cpuinfo | grep processor | wc -l)
cmake ../cmake
make $debug_make -j$corenum


if [ ! -d $output_path ]; then
    mkdir -p $output_path
fi

if [ ! -f $object_dir/$exe_name ]; then
    echo "make exe failed, please check your code."
    exit 3
fi

cp $object_dir/$exe_name $output_path/$exe_name

echo "please check whether $exe_name is the newest."
ls $project_dir/output -lahr

exit 0
