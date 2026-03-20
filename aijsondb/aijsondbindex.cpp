
#include "quickjs.h"
#include "quickjs-libc.h"
#include "aijsondblib.h"
#include "aijsondbindex.h"


const char* CREATE_INDEX_JS_1 = R"(
let buckets=aijsondb_buckets();
let nbucket=aijsondb_bucket_length(buckets[0]);
let jschema=aijsondb_schema();
function get_bucket_index_from_name(bucket_name){
   if(buckets && buckets.length>0)
   {
     for (var i=0; i<buckets.length; i++)
     {
        if (buckets[i] === bucket_name)
        {
            return i;
        }
     }
   }
   return -1;
}
)";

const char* CREATE_INDEX_JS_2 = R"( 
function generate_root_classes_from_json_schema(json_schema,get_entity_data_function_name)
{
    var ret={};
    if(json_schema["type"]!=='object'){
        return null;
    }
    for (const key in json_schema['properties']){

        let arraydesc=json_schema['properties'][key];
        if(arraydesc["type"]!=='array')
            return null;
        let entity_name="ED"+key;
        let entity_index=get_bucket_index_from_name(key);
        var source_lines=[];
        source_lines.push(`
 class ${entity_name} {
   constructor(eindex)
    {
        this.eindex=eindex;
        this.bindex=${entity_index};
    }
`
        )
        let array_object_desc=arraydesc["items"]["properties"];
        //console.log(array_object_desc)
        for (const name_prop in array_object_desc){
            source_lines.push(
`
get ${name_prop}()
    {
        let jdata=${get_entity_data_function_name}("${key}",this.eindex);
        //let jdata=JSON.parse(sData);
        return jdata.${name_prop};
    }
`);
        }
         source_lines.push(`
};
         ${entity_name}
`
        )
        ret[key]=source_lines.join('\n');
    }
    return ret;
}
)";

const char* CREATE_INDEX_JS_3 = R"( 
let root_class_scripts=generate_root_classes_from_json_schema(jschema,"get_entity_data_function");
)";

const char* CREATE_INDEX_JS_4 = R"( 
function get_entity_data_function(rootBucket,index)
{
    return aijsondb_bucket_object(rootBucket,index);
}
)";

const char* CREATE_INDEX_JS_5 = R"( 
data={};
for (const property in root_class_scripts) {
    let ClassEntity=eval(root_class_scripts[property]);
    let nentities=aijsondb_bucket_length(property);
    var employees8=[];
    for (var i=0;i<nentities;i++)
    {
       let ei=new ClassEntity(i);
       employees8.push(ei);
    }
    data[property]=employees8;
}
)";

void js_error_message(JSContext* ctx, JSValue val, char* buffer, int nbuffer) {
    if (nbuffer < 1)
        return;
    if (!JS_IsException(val))
        return;
    JSValue exception = JS_GetException(ctx);
    const char* error_message = JS_ToCString(ctx, exception);
    JS_FreeValue(ctx, exception);
    if (error_message != NULL)
    {
        if (strlen(error_message) < nbuffer - 1)
            strcpy(buffer, error_message);
        else
            buffer[0] = '\0';
        JS_FreeCString(ctx, error_message);
    }
    else
    {
        if (strlen(UNKOWN_JS_ERROR) < nbuffer - 1)
            strcpy(buffer, UNKOWN_JS_ERROR);
        else
            buffer[0] = '\0';
    }
}

int aijsondb_execute_script(JSContext* ctx, const char* script) {
    JSValue jsv = JS_Eval(ctx, script, strlen(script), "<script>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(jsv)) {
        char buffer[1024];
        js_error_message(ctx, jsv, buffer, 1024);
        printf("%s\n", buffer);
        JS_FreeValue(ctx, jsv);
        return -1;
    }
    else {
        JS_FreeValue(ctx, jsv);
        return 0;
    }
}

int aijsondb_index(JSContext* ctx) {

    int ret = 0;
	ret = aijsondb_execute_script(ctx, CREATE_INDEX_JS_1);
    if( ret != 0) {
        printf("Error executing script 1\n");
        return -1;
    }
    ret = aijsondb_execute_script(ctx, CREATE_INDEX_JS_2);
    if (ret != 0) {
        printf("Error executing script 2\n");
        return -1;
    }
    ret = aijsondb_execute_script(ctx, CREATE_INDEX_JS_3);
    if (ret != 0) {
        printf("Error executing script 3\n");
        return -1;
    }
    ret = aijsondb_execute_script(ctx, CREATE_INDEX_JS_4);
    if (ret != 0) {
        printf("Error executing script 4\n");
        return -1;
    }
    ret = aijsondb_execute_script(ctx, CREATE_INDEX_JS_5);
    if (ret != 0) {
        printf("Error executing script 5\n");
        return -1;
	}

	return 0;
}