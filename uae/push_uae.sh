
#默认版本号
module=$1
appid=$2
version=$3
envs_dir=$4

#进入脚本所在目录
basepath=$(cd `dirname $0`; pwd)
cd $basepath
#进入发布环境目录
cd ../$envs_dir/
#在子目录执行
cd ${module}
#打包
tar_name=""
if [ ! -n ${version} ] ;then
	tar_name="${module}.tgz"
else
	tar_name="${module}-${version}.tgz"
fi
rm *.tgz
if [ -d "data" ];then
	tar czf ${tar_name} etc log bin data
else
	tar czf ${tar_name} etc log bin
fi


#发布
#curl_r=`curl --user uae_system\@126.com:eJr2pUDT5AnzYTKl  -F "files[]=@${tar_name}" http://test.uae.ucweb.local/${appid}/upload.json`
curl_r=`curl --user uae_system\@126.com:eJr2pUDT5AnzYTKl  -F "files[]=@${tar_name}" http://uae.ucweb.local/${appid}/upload.json`
echo $curl_r



