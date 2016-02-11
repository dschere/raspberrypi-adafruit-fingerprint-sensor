#include <Python.h>

#include "Adafruit_Fingerprint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



/**
    python extension api
*/
static PyObject* FpSensorError = NULL;
static PyObject* CommunicationError = NULL;

static char* fpsensor_doc = (char*) "Interface to Adafruit fingerprint sensor";

static HardwareSerial HS;
static Adafruit_Fingerprint AFP( &HS );


static int check( int errcode, int expected, PyObject** exc ) 
{
    int result = -1;

    if ( *exc != NULL )
        return -1;

    if ( expected == errcode ) 
        return 0;
     
    switch( errcode ) 
    {
    // look for critical errors
    case FINGERPRINT_IMAGEMESS:
         PyErr_SetString(FpSensorError,"Image too messy");
         *exc = FpSensorError;
         break;
    case FINGERPRINT_PACKETRECIEVEERR:
         PyErr_SetString(CommunicationError,"comm error");
         *exc = CommunicationError; 
         break;
    case FINGERPRINT_FEATUREFAIL:
         PyErr_SetString(FpSensorError,"Could not find fingerprint features");
         *exc = FpSensorError;
         break;
    case FINGERPRINT_INVALIDIMAGE:
         PyErr_SetString(FpSensorError,"Invalid image");
         *exc = FpSensorError;
         break;
    //case FINGERPRINT_TIMEOUT:
    //     PyErr_SetString(FpSensorError,"Timeout");
    //     break;
    case FINGERPRINT_NOFINGER:
         result = 1;
         break;
    case FINGERPRINT_OK:
         result = 2;
         break;
    }
    return result;
}


static PyObject*
fp_wait_for_no_finger(PyObject* self, PyObject *args)
{
    int timeout = 10;
    time_t now;
    time_t expire;
    int rcode;
    int res = 0; 
    PyObject* exc = NULL;

    if (!PyArg_ParseTuple(args, "|i", &timeout))
        return NULL;

    time( &now );
    expire = now + timeout;
    do {
        rcode = AFP.getImage();
        // check for no finger
        if (check(rcode,FINGERPRINT_NOFINGER,&exc) == -1 && exc)
        {
            return exc;
        }
          
        time( &now );
        // check for timeout.
        if (now > expire)
        {
            res = -1;
            break;
        }
    } while( rcode != FINGERPRINT_NOFINGER);

    return Py_BuildValue("i",res);
}

static PyObject*
fp_getimage(PyObject* self, PyObject *args)
{
    int slot=1; // default in adafruit code
    int rcode;
    PyObject* exc = NULL;

    if (!PyArg_ParseTuple(args, "|i", &slot))
        return NULL;
    
    // get fingerprint image
    do 
    {
        rcode = AFP.getImage();
        if (check(rcode,FINGERPRINT_OK,&exc)==-1 && exc)
        {
            return exc;
        } 
    }while( rcode != FINGERPRINT_OK );

    // convert image
    rcode = AFP.image2Tz( slot );
    if (check(rcode,FINGERPRINT_OK,&exc)==-1 && exc)
    {
        return exc;
    }

    return Py_BuildValue("");
}

static PyObject* 
fp_setup(PyObject* self, PyObject *args) 
{
    char* device = (char*) "/dev/ttyAMA0";
    int baudrate = 57600;

    if (!PyArg_ParseTuple(args, "|si", &device, &baudrate))
        return NULL;

    // open the device
    if ( HS.open( device ) == -1 ) {
        PyErr_SetString(FpSensorError, "Unable to connect to serial device");
        return NULL; 
    }

    // set baud rate
    AFP.begin( baudrate );

    // perform a 'login' 
    if ( AFP.verifyPassword() == true ) {
        // connected
    } else {
        PyErr_SetString(FpSensorError, "validation failed");
        return NULL; 
    }

    // return none we aready to go.
    return Py_BuildValue("");
}

static PyObject*
fp_createmodel(PyObject* self, PyObject* args)
{
    int ident;
    PyObject* exc = NULL;    

    if (!PyArg_ParseTuple(args, "i", &ident))
        return NULL;
    
    if ( 
      (check(AFP.createModel(),FINGERPRINT_OK,&exc) == 0)
      &&
      (check(AFP.storeModel(ident),FINGERPRINT_OK,&exc) == 0)
    )
    { 
        // finger print is ok and we were able to store it 
        return Py_BuildValue("");
    }
    return exc; // exception occured.
}

static PyObject*
fp_matchmodel(PyObject* self, PyObject* args)
{
    int timeout=10;
    time_t expire, now;
    int rcode;
    PyObject* exc=NULL;

    if (!PyArg_ParseTuple(args, "i", &timeout))
        return NULL;
    
    time(&now);
    expire = now + timeout;

    while (now < expire){
        time(&now);
        rcode = AFP.getImage();
printf("AFP.getImage() -> %d\n", rcode);
        if (rcode == FINGERPRINT_NOFINGER) {
            // user is not pressing the sensor
            sleep(1);
            continue;    
        }
        else if (rcode == FINGERPRINT_OK) {
             break;
        }
        // we know that we have an error, use this function
        // to throw the correct exception.
        else if (check(rcode,FINGERPRINT_OK,&exc) == -1 && exc)
        {
            return exc;     
        }    
    }
  
    if (
         (check(AFP.image2Tz(),FINGERPRINT_OK,&exc)==0) 
         &&
         (check(AFP.fingerFastSearch(),FINGERPRINT_OK,&exc)==0)
       )
    {
        return Py_BuildValue("(i,i,i)", 1, AFP.fingerID, AFP.confidence);
    }
    else if ( exc == NULL )
    {
        return Py_BuildValue("(i,i,i)",0,0,0);
    }
    else
    {
        return exc;
    }
}

static PyObject*fp_deleteModel(PyObject* self, PyObject* args)
{
    int id;
    int rcode;
    PyObject* r = NULL;

    if (!PyArg_ParseTuple(args, "i", &id))
        return NULL;
   
    rcode=AFP.deleteModel(id);
    switch( rcode )
    {
        case FINGERPRINT_OK:
            r = Py_BuildValue("i",0);
            break;
        case FINGERPRINT_BADLOCATION:
            r = Py_BuildValue("i",-1);
            break;
        default:
            check(rcode,FINGERPRINT_OK,&r); // force an exception
            break; 
    }

    return r;
}


static PyObject* 
fp_getTemplate(PyObject* self, PyObject* args)
{
    int id;
    uint8_t templateBuffer[256];
    int index;
    PyObject* exc=NULL; 

    memset(templateBuffer, 0xff, 256);  //zero out template buffer

    if (!PyArg_ParseTuple(args, "i", &id))
        return NULL;

    if (check(AFP.loadModel(id),FINGERPRINT_OK,&exc)==-1)
        return exc;
    if (check(AFP.getModel(),FINGERPRINT_OK,&exc)==-1)
        return exc;
  
    // read a stream of bytes representing the 16x16 fingerprint template.  
    for(index = 0; index < 256; index++)
    {
        if (HS.available()){
            templateBuffer[index] = (uint8_t) HS.read();  
        }
    }
  
    return Py_BuildValue("y#",templateBuffer,sizeof(templateBuffer));
}


static PyMethodDef FPSensorMethods[] = {
//    {"system",  spam_system, METH_VARARGS,  "Execute a shell command."},
    {"deleteModel",fp_deleteModel, METH_VARARGS, "deleteModel(id) delete fingerprint model"},
    {"getTemplate",fp_getTemplate, METH_VARARGS, "getTemplate(id) -> bytes[256] -> 16x16 fpt."},
    {"matchModel",fp_matchmodel,  METH_VARARGS, "matchModel(timeout) -> (matched,id,confidence)"},
    {"createModel",fp_createmodel, METH_VARARGS, "createModel(id): associate fp image with id."},
    {"fingerRelease", fp_wait_for_no_finger, METH_VARARGS, "wait until finger not on sensor"}, 
    {"captureImage", fp_getimage, METH_VARARGS, "get image and store in a buffer"},

    {"setup", fp_setup, METH_VARARGS, "setup( device=/dev/ttyAMA0, baud=57600 );"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef fpsensormodule = {
   PyModuleDef_HEAD_INIT,
   "fpsensor",   /* name of module */
   fpsensor_doc, /* module documentation, may be NULL */
   -1,       /* size of per-interpreter state of the module,
                or -1 if the module keeps state in global variables. */
   FPSensorMethods
};



PyMODINIT_FUNC
PyInit_fpsensor(void)
{
    PyObject* m;

    m =  PyModule_Create( &fpsensormodule );
    if ( m == NULL)
        return NULL;

    FpSensorError = PyErr_NewException("fpsensor.error", NULL, NULL);
    Py_INCREF(FpSensorError);
    PyModule_AddObject(m, "error", FpSensorError); 

    CommunicationError = PyErr_NewException("fpsensor.nocomm", NULL, NULL);
    Py_INCREF(CommunicationError);
    PyModule_AddObject(m, "nocomm", CommunicationError);

    return m;
}






