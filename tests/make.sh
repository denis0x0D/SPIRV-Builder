g++ -o ir_builder_test ir_builder_test.cc -Wall -Wextra -fsanitize=address -g 
g++ -o compute VulkanRuntime.cc -lvulkan -L$VULKAN_SDK/lib -I$VULKAN_SDK/include -DDEBUG
