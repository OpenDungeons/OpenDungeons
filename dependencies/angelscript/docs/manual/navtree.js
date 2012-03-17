var NAVTREE =
[
  [ "AngelScript", "index.html", [
    [ "Manual", "index.html", [
      [ "Getting started", "doc_start.html", [
        [ "Overview", "doc_overview.html", null ],
        [ "License", "doc_license.html", null ],
        [ "Compile the library", "doc_compile_lib.html", null ],
        [ "Your first script", "doc_hello_world.html", null ],
        [ "Good practices", "doc_good_practice.html", null ]
      ] ],
      [ "Using AngelScript", "doc_using.html", [
        [ "Understanding AngelScript", "doc_understanding_as.html", [
          [ "Datatypes in AngelScript and C++", "doc_as_vs_cpp_types.html", null ],
          [ "The string type", "doc_strings.html", null ],
          [ "Arrays", "doc_arrays.html", null ],
          [ "Object handles", "doc_obj_handle.html", null ],
          [ "Memory management", "doc_memory.html", null ]
        ] ],
        [ "Registering the application interface", "doc_register_api_topic.html", [
          [ "What can be registered", "doc_register_api.html", null ],
          [ "Registering a function", "doc_register_func.html", null ],
          [ "Registering global properties", "doc_register_prop.html", null ],
          [ "Registering an object type", "doc_register_type.html", [
            [ "Registering a reference type", "doc_reg_basicref.html", null ],
            [ "Registering a value type", "doc_register_val_type.html", null ],
            [ "Registering operator behaviours", "doc_reg_opbeh.html", null ],
            [ "Registering object methods", "doc_reg_objmeth.html", null ],
            [ "Registering object properties", "doc_reg_objprop.html", null ]
          ] ],
          [ "Advanced application interface", "doc_advanced_api.html", [
            [ "Garbage collected objects", "doc_gc_object.html", null ],
            [ "The generic calling convention", "doc_generic.html", null ],
            [ "Registering a generic handle type", "doc_adv_generic_handle.html", null ],
            [ "Registering a scoped reference type", "doc_adv_scoped_type.html", null ],
            [ "Registering a single-reference type", "doc_adv_single_ref_type.html", null ],
            [ "Class hierarchies", "doc_adv_class_hierarchy.html", null ],
            [ "The variable parameter type", "doc_adv_var_type.html", null ],
            [ "Template types", "doc_adv_template.html", null ]
          ] ]
        ] ],
        [ "Compiling scripts", "doc_compile_script.html", null ],
        [ "Calling a script function", "doc_call_script_func.html", null ],
        [ "Using script classes", "doc_use_script_class.html", null ],
        [ "Advanced topics", "doc_advanced.html", [
          [ "Debugging scripts", "doc_debug.html", null ],
          [ "Timeout long running scripts", "doc_adv_timeout.html", null ],
          [ "Garbage collection", "doc_gc.html", null ],
          [ "Multithreading", "doc_adv_multithread.html", null ],
          [ "Concurrent scripts", "doc_adv_concurrent.html", null ],
          [ "Co-routines", "doc_adv_coroutine.html", null ],
          [ "Pre-compiled byte code", "doc_adv_precompile.html", null ],
          [ "Fine tuning", "doc_finetuning.html", null ],
          [ "Access masks and exposing different interfaces", "doc_adv_access_mask.html", null ],
          [ "Using namespaces", "doc_adv_namespace.html", null ],
          [ "Dynamic configurations", "doc_adv_dynamic_config.html", null ],
          [ "JIT compilation", "doc_adv_jit_topic.html", [
            [ "How to build a JIT compiler", "doc_adv_jit.html", null ],
            [ "Byte code instructions", "doc_adv_jit_1.html", null ]
          ] ]
        ] ]
      ] ],
      [ "The script language", "doc_script.html", [
        [ "Global script entities", "doc_global.html", null ],
        [ "Statements", "doc_script_statements.html", null ],
        [ "Expressions", "doc_expressions.html", null ],
        [ "Data types", "doc_datatypes.html", [
          [ "Primitives", "doc_datatypes_primitives.html", null ],
          [ "Arrays", "doc_datatypes_arrays.html", null ],
          [ "Objects and handles", "doc_datatypes_obj.html", null ],
          [ "Strings", "doc_datatypes_strings.html", null ],
          [ "Function pointers", "doc_datatypes_funcptr.html", null ]
        ] ],
        [ "Object handles", "doc_script_handle.html", null ],
        [ "Script classes", "doc_script_class.html", [
          [ "Script class overview", "doc_script_class_desc.html", null ],
          [ "Inheritance and polymorphism", "doc_script_class_inheritance.html", null ],
          [ "Private class members", "doc_script_class_private.html", null ],
          [ "Operator overloads", "doc_script_class_ops.html", null ]
        ] ],
        [ "Property accessors", "doc_script_class_prop.html", null ],
        [ "Returning a reference", "doc_script_retref.html", null ],
        [ "Shared script entities", "doc_script_shared.html", null ],
        [ "Operator precedence", "doc_operator_precedence.html", null ],
        [ "Reserved keywords and tokens", "doc_reserved_keywords.html", null ]
      ] ],
      [ "The API reference", "doc_api.html", [
        [ "Functions", "doc_api_functions.html", null ],
        [ "Interfaces", "doc_api_interfaces.html", null ]
      ] ],
      [ "Samples", "doc_samples.html", [
        [ "Tutorial", "doc_samples_tutorial.html", null ],
        [ "Concurrent scripts", "doc_samples_concurrent.html", null ],
        [ "Console", "doc_samples_console.html", null ],
        [ "Co-routines", "doc_samples_corout.html", null ],
        [ "Events", "doc_samples_events.html", null ],
        [ "Include directive", "doc_samples_incl.html", null ],
        [ "Generic compiler", "doc_samples_asbuild.html", null ],
        [ "Commandline runner", "doc_samples_asrun.html", null ],
        [ "Game", "doc_samples_game.html", null ]
      ] ],
      [ "Add-ons", "doc_addon.html", [
        [ "Application modules", "doc_addon_application.html", [
          [ "Script builder", "doc_addon_build.html", null ],
          [ "Context manager", "doc_addon_ctxmgr.html", null ],
          [ "Debugger", "doc_addon_debugger.html", null ],
          [ "Serializer", "doc_addon_serializer.html", null ],
          [ "Helper functions", "doc_addon_helpers.html", null ],
          [ "Automatic wrapper functions", "doc_addon_autowrap.html", null ]
        ] ],
        [ "Script extensions", "doc_addon_script.html", [
          [ "string object", "doc_addon_std_string.html", null ],
          [ "array template object", "doc_addon_array.html", null ],
          [ "any object", "doc_addon_any.html", null ],
          [ "ref object", "doc_addon_handle.html", null ],
          [ "dictionary object", "doc_addon_dict.html", null ],
          [ "file object", "doc_addon_file.html", null ],
          [ "math functions", "doc_addon_math.html", null ]
        ] ]
      ] ]
    ] ],
    [ "Related Pages", "pages.html", [
      [ "Todo List", "todo.html", null ]
    ] ],
    [ "Class List", "annotated.html", [
      [ "asIBinaryStream", "classas_i_binary_stream.html", null ],
      [ "asIJITCompiler", "classas_i_j_i_t_compiler.html", null ],
      [ "asIObjectType", "classas_i_object_type.html", null ],
      [ "asIScriptContext", "classas_i_script_context.html", null ],
      [ "asIScriptEngine", "classas_i_script_engine.html", null ],
      [ "asIScriptFunction", "classas_i_script_function.html", null ],
      [ "asIScriptGeneric", "classas_i_script_generic.html", null ],
      [ "asIScriptModule", "classas_i_script_module.html", null ],
      [ "asIScriptObject", "classas_i_script_object.html", null ],
      [ "asSBCInfo", "structas_s_b_c_info.html", null ],
      [ "asSFuncPtr", "structas_s_func_ptr.html", null ],
      [ "asSMessageInfo", "structas_s_message_info.html", null ],
      [ "asSVMRegisters", "structas_s_v_m_registers.html", null ]
    ] ],
    [ "Class Index", "classes.html", null ],
    [ "Class Members", "functions.html", null ],
    [ "File List", "files.html", [
      [ "angelscript.h", "angelscript_8h.html", null ]
    ] ],
    [ "File Members", "globals.html", null ]
  ] ]
];

function createIndent(o,domNode,node,level)
{
  if (node.parentNode && node.parentNode.parentNode)
  {
    createIndent(o,domNode,node.parentNode,level+1);
  }
  var imgNode = document.createElement("img");
  if (level==0 && node.childrenData)
  {
    node.plus_img = imgNode;
    node.expandToggle = document.createElement("a");
    node.expandToggle.href = "javascript:void(0)";
    node.expandToggle.onclick = function() 
    {
      if (node.expanded) 
      {
        $(node.getChildrenUL()).slideUp("fast");
        if (node.isLast)
        {
          node.plus_img.src = node.relpath+"ftv2plastnode.png";
        }
        else
        {
          node.plus_img.src = node.relpath+"ftv2pnode.png";
        }
        node.expanded = false;
      } 
      else 
      {
        expandNode(o, node, false);
      }
    }
    node.expandToggle.appendChild(imgNode);
    domNode.appendChild(node.expandToggle);
  }
  else
  {
    domNode.appendChild(imgNode);
  }
  if (level==0)
  {
    if (node.isLast)
    {
      if (node.childrenData)
      {
        imgNode.src = node.relpath+"ftv2plastnode.png";
      }
      else
      {
        imgNode.src = node.relpath+"ftv2lastnode.png";
        domNode.appendChild(imgNode);
      }
    }
    else
    {
      if (node.childrenData)
      {
        imgNode.src = node.relpath+"ftv2pnode.png";
      }
      else
      {
        imgNode.src = node.relpath+"ftv2node.png";
        domNode.appendChild(imgNode);
      }
    }
  }
  else
  {
    if (node.isLast)
    {
      imgNode.src = node.relpath+"ftv2blank.png";
    }
    else
    {
      imgNode.src = node.relpath+"ftv2vertline.png";
    }
  }
  imgNode.border = "0";
}

function newNode(o, po, text, link, childrenData, lastNode)
{
  var node = new Object();
  node.children = Array();
  node.childrenData = childrenData;
  node.depth = po.depth + 1;
  node.relpath = po.relpath;
  node.isLast = lastNode;

  node.li = document.createElement("li");
  po.getChildrenUL().appendChild(node.li);
  node.parentNode = po;

  node.itemDiv = document.createElement("div");
  node.itemDiv.className = "item";

  node.labelSpan = document.createElement("span");
  node.labelSpan.className = "label";

  createIndent(o,node.itemDiv,node,0);
  node.itemDiv.appendChild(node.labelSpan);
  node.li.appendChild(node.itemDiv);

  var a = document.createElement("a");
  node.labelSpan.appendChild(a);
  node.label = document.createTextNode(text);
  a.appendChild(node.label);
  if (link) 
  {
    a.href = node.relpath+link;
  } 
  else 
  {
    if (childrenData != null) 
    {
      a.className = "nolink";
      a.href = "javascript:void(0)";
      a.onclick = node.expandToggle.onclick;
      node.expanded = false;
    }
  }

  node.childrenUL = null;
  node.getChildrenUL = function() 
  {
    if (!node.childrenUL) 
    {
      node.childrenUL = document.createElement("ul");
      node.childrenUL.className = "children_ul";
      node.childrenUL.style.display = "none";
      node.li.appendChild(node.childrenUL);
    }
    return node.childrenUL;
  };

  return node;
}

function showRoot()
{
  var headerHeight = $("#top").height();
  var footerHeight = $("#nav-path").height();
  var windowHeight = $(window).height() - headerHeight - footerHeight;
  navtree.scrollTo('#selected',0,{offset:-windowHeight/2});
}

function expandNode(o, node, imm)
{
  if (node.childrenData && !node.expanded) 
  {
    if (!node.childrenVisited) 
    {
      getNode(o, node);
    }
    if (imm)
    {
      $(node.getChildrenUL()).show();
    } 
    else 
    {
      $(node.getChildrenUL()).slideDown("fast",showRoot);
    }
    if (node.isLast)
    {
      node.plus_img.src = node.relpath+"ftv2mlastnode.png";
    }
    else
    {
      node.plus_img.src = node.relpath+"ftv2mnode.png";
    }
    node.expanded = true;
  }
}

function getNode(o, po)
{
  po.childrenVisited = true;
  var l = po.childrenData.length-1;
  for (var i in po.childrenData) 
  {
    var nodeData = po.childrenData[i];
    po.children[i] = newNode(o, po, nodeData[0], nodeData[1], nodeData[2],
        i==l);
  }
}

function findNavTreePage(url, data)
{
  var nodes = data;
  var result = null;
  for (var i in nodes) 
  {
    var d = nodes[i];
    if (d[1] == url) 
    {
      return new Array(i);
    }
    else if (d[2] != null) // array of children
    {
      result = findNavTreePage(url, d[2]);
      if (result != null) 
      {
        return (new Array(i).concat(result));
      }
    }
  }
  return null;
}

function initNavTree(toroot,relpath)
{
  var o = new Object();
  o.toroot = toroot;
  o.node = new Object();
  o.node.li = document.getElementById("nav-tree-contents");
  o.node.childrenData = NAVTREE;
  o.node.children = new Array();
  o.node.childrenUL = document.createElement("ul");
  o.node.getChildrenUL = function() { return o.node.childrenUL; };
  o.node.li.appendChild(o.node.childrenUL);
  o.node.depth = 0;
  o.node.relpath = relpath;

  getNode(o, o.node);

  o.breadcrumbs = findNavTreePage(toroot, NAVTREE);
  if (o.breadcrumbs == null)
  {
    o.breadcrumbs = findNavTreePage("index.html",NAVTREE);
  }
  if (o.breadcrumbs != null && o.breadcrumbs.length>0)
  {
    var p = o.node;
    for (var i in o.breadcrumbs) 
    {
      var j = o.breadcrumbs[i];
      p = p.children[j];
      expandNode(o,p,true);
    }
    p.itemDiv.className = p.itemDiv.className + " selected";
    p.itemDiv.id = "selected";
    $(window).load(showRoot);
  }
}

