import torch
import torch_npu
from torch_npu.testing.testcase import TestCase, run_tests
import custom_ops_lib
torch.npu.config.allow_internal_format = False
import numpy as np
import sys
import re

def gen_data(shape, dtype, low=-10, high=10):
    if dtype == np.bool_:
        return np.random.randint(0, 2, shape).astype(dtype)
    elif dtype in [np.float16, np.float32]:
        return np.random.uniform(low, high, shape).astype(dtype)
    else:
        return np.random.randint(low, high, shape).astype(dtype)

case_data = {}

# 1. bool
case_data['case1'] = {'input': gen_data([64], np.bool_), 'other': gen_data([64], np.bool_)}
case_data['case2'] = {'input': gen_data([1027], np.bool_), 'other': gen_data([1027], np.bool_)}

# 2. float32
case_data['case3'] = {'input': gen_data([64], np.float32), 'other': gen_data([64], np.float32)}
case_data['case4'] = {'input': gen_data([1027], np.float32), 'other': gen_data([1027], np.float32)}

# 3. float16
case_data['case5'] = {'input': gen_data([64], np.float16), 'other': gen_data([64], np.float16)}
case_data['case6'] = {'input': gen_data([1027], np.float16), 'other': gen_data([1027], np.float16)}

# 4. bfloat16
# case_data['case7'] 
# case_data['case8'] 

# 5. int64
# case_data['case9'] 
# case_data['case10'] 

# 6. int32
case_data['case11'] = {'input': gen_data([64], np.int32, -100, 100), 'other': gen_data([64], np.int32, -100, 100)}
case_data['case12'] = {'input': gen_data([1027], np.int32, -100, 100), 'other': gen_data([1027], np.int32, -100, 100)}

# 7. int16
case_data['case13'] = {'input': gen_data([64], np.int16), 'other': gen_data([64], np.int16)}
case_data['case14'] = {'input': gen_data([1027], np.int16), 'other': gen_data([1027], np.int16)}

# 8. int8
case_data['case15'] = {'input': gen_data([64], np.int8), 'other': gen_data([64], np.int8)}
case_data['case16'] = {'input': gen_data([1027], np.int8), 'other': gen_data([1027], np.int8)}

# 9. uint8
case_data['case17'] = {'input': gen_data([64], np.uint8, 0, 20), 'other': gen_data([64], np.uint8, 0, 20)}
case_data['case18'] = {'input': gen_data([1027], np.uint8, 0, 20), 'other': gen_data([1027], np.uint8, 0, 20)}

# Broadcast
case_data['case19'] = {'input': gen_data([1, 64], np.float16), 'other': gen_data([64, 64], np.float16)}


def verify_result(real_result, golden):
    if golden.dtype == torch.float32:
        rtol = 1e-4
        atol = 1e-4
    elif golden.dtype == torch.float16:
        rtol = 1e-3
        atol = 1e-3
    else:
        rtol = 0
        atol = 0

    if golden.dtype in [torch.float16, torch.float32]:
        minimum = 10e-10
        golden = torch.where(golden == 0, minimum, golden)
        real_result = torch.where(real_result == 0, minimum, real_result)
        abs_diff = torch.abs(real_result - golden)
        rel_diff = abs_diff / torch.max(torch.abs(real_result), torch.abs(golden))
        is_close = (abs_diff <= atol) | (rel_diff <= rtol)
        both_nan = torch.isnan(real_result) & torch.isnan(golden)
        is_close = is_close | both_nan
    else:
        is_close = (real_result == golden)
        
    err_num = torch.sum(~is_close).item()

    if real_result.numel() * rtol < err_num and err_num > 0:
        print(f"[ERROR] result error. Errors: {err_num}/{real_result.numel()}")
        return False
    print("test pass")
    return True

    
class TestCustomOP(TestCase):
    def test_custom_op_case(self,num):
        print(f"Testing case {num}")
        caseNmae='case'+str(num) 
        
        if caseNmae not in case_data:
            print(f"{caseNmae} not found in case_data")
            return

        input_x_np = case_data[caseNmae]["input"]
        input_other_np = case_data[caseNmae]["other"]
        
        input_x = torch.from_numpy(input_x_np)
        input_other = torch.from_numpy(input_other_np)
        
        output = torch.fmax(input_x, input_other)
        
        try:
            output_npu = custom_ops_lib.custom_op(input_x.npu(), input_other.npu())
        except Exception as e:
            print(f"{caseNmae} execution failed: {e}")
            return

        if output_npu is None:
            print(f"{caseNmae} execution timed out!")
        else:
            if verify_result(output_npu.cpu(), output):
                print(f"{caseNmae} verify result pass!")
            else:
                print(f"{caseNmae} verify result failed!")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        TestCustomOP().test_custom_op_case(sys.argv[1])
    else:
        # Run all cases sorted by case number
        def extract_case_num(key):
            match = re.search(r'case(\d+)', key)
            return int(match.group(1)) if match else 0
            
        sorted_keys = sorted(case_data.keys(), key=extract_case_num)
        
        print(f"Running all {len(sorted_keys)} cases...")
        for key in sorted_keys:
            num = extract_case_num(key)
            TestCustomOP().test_custom_op_case(num)
