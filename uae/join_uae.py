#!/usr/local/bin/python
# -*- coding:utf-8 -*-
# 生成 uae 依赖 的脚本
# conf

import ConfigParser
import  sys,os

base_dir="../"

def ex_sh(shell):
    print(shell)
    os.system("cd "+base_dir+";"+shell)

if __name__ == '__main__':
    conf_file="join_uae.ini"
    print("base_dir:%s\nconf_file:%s"%(base_dir,conf_file))
    conf = ConfigParser.ConfigParser()
    conf.read(conf_file)
    #cover=conf.get("main","cover")
    appid=conf.get("main","appid")
    appid_pre=conf.get("main","appid_pre")
    code_dir=conf.get("main","code_dir")
    targ=conf.get("main","targ")
    common_dirs=conf.get("main","common_dirs")
    conf_dir=conf.get("main","conf_dir")
    conf_dir_pre=conf.get("main","conf_dir_pre")
    makefile_template=conf.get("main","makefile_template")
    dir_deep=len(code_dir.split("/"))
    tier="../"
    for i in range(1,dir_deep):
        tier=tier+"../"
    print(tier)
    # 需要创建的文件列表：
    file_list=["envs/${targ}/bin/",
               "envs/${targ}/etc/",
               "envs/${targ}/log/",
               "pre_envs/${targ}/bin/",
               "pre_envs/${targ}/etc/",
               "pre_envs/${targ}/log/"
               ]
    help_str=""
    for file_dir in file_list:
        file_dir=file_dir.replace("${targ}",targ)
        ex_sh(" mkdir -p %s; touch %shandle"%(file_dir,file_dir))
        help_str=help_str+"git add "+base_dir+file_dir+"\n"

    ex_sh(" cp  "+conf_dir_pre+"/*  pre_envs/%s/etc/"%targ )
    ex_sh(" cp  "+conf_dir+"/*  envs/%s/etc/"%targ )

    # print(file_list)
    make_need_options=conf.options("make_need")
    make_need_str=""
    for option in make_need_options:
        make_need_str=make_need_str+option+"="+conf.get("make_need",option)+"\n"
    # print(make_need_str)
    template_content=open(makefile_template).read()

    template_content=template_content.replace("${appid}",appid)
    template_content=template_content.replace("${targ}",targ)
    template_content=template_content.replace("${appid_pre}",appid_pre)
    template_content=template_content.replace("${make_need}",make_need_str.upper())
    template_content=template_content.replace("${tier}",tier)
    template_content=template_content.replace("${common_dirs}",common_dirs)

    makefile_path=code_dir+"/Makefile"
    open(base_dir+makefile_path,"w").write(template_content)
    print(" mod file: "+makefile_path)
    help_str=help_str+"git add "+base_dir+makefile_path+"\n"
    print("\n please add file to git \n"+help_str)


    # os.system("make dir ")






