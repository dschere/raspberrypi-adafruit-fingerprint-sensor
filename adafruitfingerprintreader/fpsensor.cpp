#include <Python.h>

#include "Adafruit_Fingerprint.h"
#include "enroll.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



/**
    python extension api
*/

static HardwareSerial HS;
static Adafruit_Fingerprint AFP( &HS );


static PyObject* FpSensorError = NULL;
static PyObject* CommunicationError = NULL;


static char* fpsensor_doc = (char*) "Interface to Adafruit fingerprint sensor";



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
    int timeout = 3;
    time_t expire, now;

    if (!PyArg_ParseTuple(args, "|ii", &slot, &timeout))
        return NULL;
    

    time(&now);
    expire = now + timeout;
    // get fingerprint image
    do 
    {
        rcode = AFP.getImage();
        if (check(rcode,FINGERPRINT_OK,&exc)==-1 && exc)
        { 
            return exc;
        } 
        rcode = AFP.image2Tz( slot );      
          
        
        time(&now);
        if ( now > expire ) {
            fprintf(stderr,"timeout in fp_getImage\n");
            PyErr_SetString(PyExc_TimeoutError,"timeout waiting for image"); 
            return PyExc_TimeoutError;           
        }
        usleep( 1000 );
    }while( rcode != FINGERPRINT_OK );

/*
    // convert image
    rcode = AFP.image2Tz( slot );
    if (check(rcode,FINGERPRINT_OK,&exc)==-1 && exc)
    {
        return exc;
    }
*/
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
    //int timeout=10;
    //time_t expire, now;
    //int rcode;

    if (
        (AFP.getImage()       == FINGERPRINT_OK) &&
        (AFP.image2Tz()       == FINGERPRINT_OK) &&
        (AFP.fingerFastSearch() == FINGERPRINT_OK)
       )
    {
        return Py_BuildValue("(i,i,i)", 1, AFP.fingerID, AFP.confidence);
    } 
    else
    {
        return Py_BuildValue("(i,i,i)", 0, 0, 0 );
    }

/*
    PyObject* exc=NULL;

    if (!PyArg_ParseTuple(args, "i", &timeout))
        return NULL;
    
    time(&now);
    expire = now + timeout;

    while (1){

        time(&now);
        if ( now > expire ) {
            PyErr_SetString(PyExc_TimeoutError,"timeout waiting for image");
            return PyExc_TimeoutError;
        }

        rcode = AFP.getImage();
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
*/

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
fp_uploadTemplate(PyObject* self, PyObject* args)
{
    int id;
    uint8_t* templateBuffer;
    int size, i, r;
    PyObject* list;
    

    if (!PyArg_ParseTuple(args, "iO", &id,&list))
        return NULL;
    
    templateBuffer = (uint8_t*) malloc( PyList_Size(list) ); 
    for (i = 0; i < PyList_Size(list); i++){
        uint8_t ch = (uint8_t) PyLong_AsLong( PyList_GET_ITEM( list, i ) );
        templateBuffer[i] = ch;
    }

    r = AFP.uploadModel(id, templateBuffer, PyList_Size(list) );
    free( templateBuffer );
    return Py_BuildValue("i",r);
}

static PyObject* 
fp_getTemplate(PyObject* self, PyObject* args)
{
    int id;
    int index;
    PyObject* exc=NULL; 
    int timeout = 2;
    time_t now, expire;
    PyObject* model;
    uint8_t packet[256];
    int i; 

    //memset(templateBuffer, 0xff, 256);  //zero out template buffer

    if (!PyArg_ParseTuple(args, "i|i", &id,&timeout))
        return NULL;

    if (check(AFP.loadModel(id),FINGERPRINT_OK,&exc)==-1)
        return (exc) ? exc : Py_BuildValue("");
    if (check(AFP.getModel(),FINGERPRINT_OK,&exc)==-1)
        return (exc) ? exc : Py_BuildValue("");
  
    time( &now );
    expire = now + timeout;
    model = PyList_New(0);

    // read a stream of bytes representing the 16x16 fingerprint template.  
    do
    {

        uint8_t len = AFP.getReply(packet);
        

        if ( packet[0] == FINGERPRINT_DATAPACKET || packet[0] == FINGERPRINT_ENDDATAPACKET  ) {
            for(i = 0; i < len; i++)
                PyList_Append( model, PyLong_FromLong( packet[i] ) );      
        }


//        if ( HS.available() ){
//            int r = HS.read();
 
//            templateBuffer[index] = (uint8_t) r;
//            index++;
//        } 

        time(&now);
        if ( now > expire ) {
            PyErr_SetString(PyExc_TimeoutError,"timeout waiting for image");
            return PyExc_TimeoutError;
        }

    }while( packet[0] != FINGERPRINT_ENDDATAPACKET  );
  
    //HS.reset();
    
    //return Py_BuildValue("y#",templateBuffer,sizeof(templateBuffer));
    return model;
}

/*
static PyObject* 
_fp_getTemplate(PyObject* self, PyObject* args)
{
    int id;
    uint8_t templateBuffer[256];
    int index;
    PyObject* exc=NULL; 
    int timeout = 2;
    time_t now, expire;
 
    memset(templateBuffer, 0xff, 256);  //zero out template buffer

    if (!PyArg_ParseTuple(args, "i|i", &id,&timeout))
        return NULL;

    if (check(AFP.loadModel(id),FINGERPRINT_OK,&exc)==-1)
        return (exc) ? exc : Py_BuildValue("");
    if (check(AFP.getModel(),FINGERPRINT_OK,&exc)==-1)
        return (exc) ? exc : Py_BuildValue("");
  
    time( &now );
    expire = now + timeout;
    // read a stream of bytes representing the 16x16 fingerprint template.  
    for(index = 0; index < 256;)
    {
        if ( HS.available() ){
            int r = HS.read();
 
            templateBuffer[index] = (uint8_t) r;
            index++;
        } 

        time(&now);
        if ( now > expire ) {
            PyErr_SetString(PyExc_TimeoutError,"timeout waiting for image");
            return PyExc_TimeoutError;
        }

    }
  
    //HS.reset();
    
    return Py_BuildValue("y#",templateBuffer,sizeof(templateBuffer));
}
*/

static PyObject* getFingerprintEnroll_event_cb = NULL;

static void getFingerprintEnroll_emit(const char* event)
{
   if ( getFingerprintEnroll_event_cb )
   {
      // call python callback within C++
      PyObject* arglist = Py_BuildValue("(s)", event);
      PyObject* result = 
          PyObject_CallObject(getFingerprintEnroll_event_cb, arglist);
      Py_DECREF(arglist);
      if ( result ) 
         Py_DECREF(result);
   }
}

//uint8_t getFingerprintEnroll(HardwareSerial& Serial, Adafruit_Fingerprint& finger, uint8_t id)
static PyObject*
fp_getFingerprintEnroll(PyObject* self, PyObject* args)
{
    uint8_t id;
    int _id;
    PyObject* result;

    if (!PyArg_ParseTuple(args, "i|O", &_id,&getFingerprintEnroll_event_cb))
        return NULL;

    id = (uint8_t)_id;
    
    if (getFingerprintEnroll_event_cb)
    {
       // prevent garbage collector from deleting pythonfunction when it passes out
       // of scope.
       Py_INCREF(getFingerprintEnroll_event_cb);
    }

    result =  Py_BuildValue("i", getFingerprintEnroll( HS, AFP, id, getFingerprintEnroll_emit ));

    if (getFingerprintEnroll_event_cb)
    {
       // allow to be garbage collected.
       Py_DECREF(getFingerprintEnroll_event_cb);
       // remove stale pointer reference.
       getFingerprintEnroll_event_cb = NULL;
    }

    return result;
}
/*
void writePacket(uint32_t addr, uint8_t packettype, uint16_t len, uint8_t *packet);
  uint8_t getReply(uint8_t packet[], uint16_t timeout=DEFAULTTIMEOUT);
*/
static PyObject* 
fp_downloadImage(PyObject* self, PyObject* args)
{
#define FINGERPRINT_DOWNLOADIMAGE  0x0A

     uint32_t addr = 0xFFFFFFFF;
     uint8_t packettype = FINGERPRINT_COMMANDPACKET;
     uint8_t packet[] = { FINGERPRINT_DOWNLOADIMAGE };
     uint8_t reply[20];
     PyObject* output = PyList_New(0);
     uint8_t type, payload, r;
     int x = 0; 
     int y = 0;
     int i, len;

printf("inside fp_downloadImage\n");

     // initiate streaming transfer
     AFP.writePacket( addr, packettype, sizeof(packet), packet );

     // get status
     memset(reply,0,sizeof(reply));
     len = AFP.getReply( reply ); 
     type = reply[0];
     payload = reply[1];

printf("waypoint 1\n");

//     if ( (type != FINGERPRINT_ACKPACKET) ||  (payload != FINGERPRINT_OK) ) 
     if ( type != FINGERPRINT_ACKPACKET )
     {
printf("type = 0x%02X, payload=0x%02X len=%d\n", type, payload, len );

         PyErr_SetString(FpSensorError, "Unable to download image");                           
         Py_DECREF( output );
         return FpSensorError;
     }
         
printf("waypoint 2\n");

     while ( type != FINGERPRINT_ENDDATAPACKET ) {

         memset(reply,0,sizeof(reply));
         len = AFP.getReply( reply, 60 );
         type = reply[0];

         if ( type != FINGERPRINT_DATAPACKET && type != FINGERPRINT_ENDDATAPACKET )
         {

printf("2 type = 0x%02X, len=%d\n", type, len );

             PyErr_SetString(FpSensorError, "Unexpected data type in octet stream ");    
             Py_DECREF( output );
             return FpSensorError;
         }
 

printf("got %d bytes\n", len);

         x = 0;        
         for(i = 1; i < len; i++) 
         {         
             int v = (reply[i] >> 4) * 17;

             PyList_Append( output, Py_BuildValue("(i,i,i)", x, y, v ) ); 

             x += 1;
             v = (reply[i] & 0xFF) * 17;

             PyList_Append( output, Py_BuildValue("(i,i,i)", x, y, v ) );

             x += 1;
         }
         y += 1;
 
    }

  
    return output;    
}
 

static PyMethodDef FPSensorMethods[] = {
//    {"system",  spam_system, METH_VARARGS,  "Execute a shell command."},
    {"getFingerprintEnroll", fp_getFingerprintEnroll, METH_VARARGS, "enroll example from adafruit."},
    {"deleteModel",fp_deleteModel, METH_VARARGS, "deleteModel(id) delete fingerprint model"},
    {"getTemplate",fp_getTemplate, METH_VARARGS, "getTemplate(id) -> bytes[256] -> 16x16 fpt."},
    {"matchModel",fp_matchmodel,  METH_NOARGS, "matchModel() -> (matched,id,confidence)"},
    {"createModel",fp_createmodel, METH_VARARGS, "createModel(id): associate fp image with id."},
    {"fingerRelease", fp_wait_for_no_finger, METH_VARARGS, "wait until finger not on sensor"}, 
    {"captureImage", fp_getimage, METH_VARARGS, "captureImage(slot=1,timeout=10); get image and store in a buffer"},

    {"setup", fp_setup, METH_VARARGS, "setup( device=/dev/ttyAMA0, baud=57600 );"},
    {"downloadImage", fp_downloadImage, METH_NOARGS, "retreive image data from flash as a bitstream"},
    {"uploadTemplate", fp_uploadTemplate, METH_VARARGS, "upload a template from host machine."},

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






