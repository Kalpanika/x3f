import hashlib
import os.path
import subprocess
import os
import time


def get_dist_name():
    # since the executable isn't in a guaranteed spot on various platforms,
    # have to find the executable to run.  Going to only run the first one found.
    # note that this approach requires that the makefile make dist as part of 
    # making check
    all_found_executables = subprocess.check_output(['find', './dist', '-executable', '-type', 'f'])
    found_executable = all_found_executables.split('\n')[0]
    print(found_executable)  # print statements are only executed by behave 
    # when the behavior fails, so this print is usually silenced if all is well
    return found_executable


def run_conversion(args):
    print(args)
    running_proc = subprocess.Popen(args)
    while running_proc.poll() is None:
        time.sleep(0.1)


@given(u'an input image {image} without a {converted_image}')
def step_impl(context, image, converted_image):
    assert os.path.isfile(image)
    if os.path.isfile(converted_image):
        os.chmod(converted_image, 0666)
        os.remove(converted_image)


@when(u'the {image} is converted by the code to {file_type}')
def step_impl(context, image, file_type):
    found_executable = get_dist_name()
    file_flag = '-dng'  # the default
    if file_type == 'TIFF':
        file_flag = '-tiff'
    if file_type == 'JPG':
        file_flag = '-jpg'
    if file_type == 'PPM':
        file_flag = '-ppm'
    args = [found_executable, file_flag, image]
    run_conversion(args)


@when(u'the {image} is denoised and converted by the code')
def step_impl(context, image):
    found_executable = get_dist_name()
    args = [found_executable, '-dng', '-denoise', image]
    run_conversion(args)


@then(u'the {converted_image} has the right {md5} hash value')
def step_impl(context, converted_image, md5):
    assert os.path.isfile(converted_image)
    with open(converted_image) as ci:
        found_hash = hashlib.md5(ci.read()).hexdigest()
        print("found_hash: ", found_hash, " expected_hash: ", md5)
        assert md5 == found_hash
    os.chmod(converted_image, 0666)
    os.remove(converted_image)  # normally, I'd remove this file in the environment
    # however, if these files should always be removed, then remove them immediately after
    # the test should be sufficient.  This should be the last 'then' statement
    # if more tests are later made.
