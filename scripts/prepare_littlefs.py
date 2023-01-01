
Import("env", "projenv")

import os
import shutil


def copy_treadmill_txt(source, target, env):
    print("Hello PlatformIO ... extra script")

    config_dir  = os.path.join(env.get('PROJECT_DIR'), 'config/')
    data_dir    = os.path.join(env.get('PROJECT_DIR'), 'data/')
    proj_config = env.GetProjectConfig()
    treadmill_model = proj_config.get("common_env_data", "treadmill")
    treadmill_config_file = "treadmill_" + treadmill_model + ".txt"
    
    print(f"config dir: {config_dir}, data: {data_dir}")
    print(f"treadmill model: {treadmill_model}")

    if os.path.exists(config_dir + treadmill_config_file):
        print("copy {} to {}".format(config_dir + treadmill_config_file, data_dir + "treadmill.txt"))
        shutil.copy(config_dir + treadmill_config_file, data_dir + "treadmill.txt")
    else:
        print("no config file: {}".format(config_dir + treadmill_config_file))

env.AddPreAction('$BUILD_DIR/littlefs.bin', copy_treadmill_txt)

