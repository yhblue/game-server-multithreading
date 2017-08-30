#include <Python.h>

int main()  
{  
    Py_Initialize();//主要是初始化python解释器
    PyRun_SimpleString("import sys");//相当于在python中的import sys语句
    PyRun_SimpleString("sys.path.append('./')");//是将搜索路径设置为当前目录
    PyObject * pModule=NULL;
    PyObject * pFunc=NULL;
    pModule = PyImport_ImportModule("helloWorld");//是利用导入文件函数将helloWorld.py函数导入
    pFunc = PyObject_GetAttrString(pModule,"hello");//是在pyton引用模块helloWorld.py中查找hello函数
    PyEval_CallObject(pFunc,NULL);//调用hello模块
    Py_Finalize()；//清理python环境释放资源
   
    return 0;  
} 


