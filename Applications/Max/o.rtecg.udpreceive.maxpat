{
	"patcher" : 	{
		"fileversion" : 1,
		"appversion" : 		{
			"major" : 7,
			"minor" : 3,
			"revision" : 4,
			"architecture" : "x64",
			"modernui" : 1
		}
,
		"rect" : [ 641.0, 79.0, 1245.0, 382.0 ],
		"bglocked" : 0,
		"openinpresentation" : 0,
		"default_fontsize" : 12.0,
		"default_fontface" : 0,
		"default_fontname" : "Arial",
		"gridonopen" : 1,
		"gridsize" : [ 15.0, 15.0 ],
		"gridsnaponopen" : 1,
		"objectsnaponopen" : 1,
		"statusbarvisible" : 2,
		"toolbarvisible" : 1,
		"lefttoolbarpinned" : 0,
		"toptoolbarpinned" : 0,
		"righttoolbarpinned" : 0,
		"bottomtoolbarpinned" : 0,
		"toolbars_unpinned_last_save" : 0,
		"tallnewobj" : 0,
		"boxanimatetime" : 200,
		"enablehscroll" : 1,
		"enablevscroll" : 1,
		"devicewidth" : 0.0,
		"description" : "",
		"digest" : "",
		"tags" : "",
		"style" : "",
		"subpatcher_template" : "",
		"boxes" : [ 			{
				"box" : 				{
					"fontface" : 0,
					"fontsize" : 12.0,
					"id" : "obj-4",
					"maxclass" : "o.display",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 502.0, 82.0, 563.0, 34.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-7",
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 112.0, 82.0, 24.0, 24.0 ],
					"style" : ""
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-3",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 286.5, 118.0, 144.0, 20.0 ],
					"style" : "",
					"text" : "udpsend localhost 40000"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 0,
					"fontsize" : 12.0,
					"id" : "obj-2",
					"linecount" : 2,
					"maxclass" : "o.expr.codebox",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "FullPacket", "FullPacket" ],
					"patching_rect" : [ 144.0, 193.0, 416.0, 50.0 ],
					"text" : "assign(/self + \"/sys/time/arrival\", /sys/time/arrival),\ndelete(/sys/time/arrival)"
				}

			}
, 			{
				"box" : 				{
					"comment" : "",
					"id" : "obj-12",
					"index" : 0,
					"maxclass" : "outlet",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 144.0, 308.0, 30.0, 30.0 ],
					"style" : ""
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-11",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "FullPacket" ],
					"patching_rect" : [ 144.0, 156.0, 92.0, 20.0 ],
					"style" : "",
					"text" : "o.rtecg.lncache"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 0,
					"fontsize" : 12.0,
					"id" : "obj-10",
					"linecount" : 3,
					"maxclass" : "o.compose",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 576.0, 261.0, 649.0, 51.0 ],
					"saved_bundle_data" : [ 35, 98, 117, 110, 100, 108, 101, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -92, 47, 97, 98, 47, 101, 99, 103, 0, 44, 105, 116, 105, 105, 105, 105, 105, 116, 105, 100, 105, 116, 105, 100, 100, 100, 100, 100, 100, 100, 100, 0, 0, 0, 0, -86, 56, -36, -111, 124, 124, -58, 28, 37, -49, 0, 0, 0, -56, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -96, -13, 111, 24, 119, 46, 84, -20, -90, -122, 0, 0, 0, 0, 63, -48, 0, 0, 0, 0, 0, 0, 0, 0, -95, 12, 91, 17, -89, 93, 13, -44, -91, -33, 0, 0, -67, 39, 63, -24, 0, 0, 0, 0, 0, 0, 64, 109, 105, 104, 114, -80, 32, -59, 64, 75, 57, -18, -53, -5, 21, -75, 64, 81, -6, 63, 20, 18, 5, -68, 64, 88, 19, -109, -35, -105, -10, 43, 64, 72, 19, -107, -127, 6, 36, -35, 64, -46, 9, 32, 0, 0, 0, 0, 64, -62, 9, 29, 112, -93, -41, 10 ],
					"saved_bundle_length" : 184,
					"text" : "/ab/ecg : [43576, 2017-04-07T02:48:28.773867Z, 200, 9, 0, 0, 41203, 1959-01-23T14:52:30.331736Z, 0, 0.25, 41228, 1948-05-31T20:06:53.054026Z, 48423, 0.75, 235.294, 54.4526, 71.9101, 96.3059, 48.153, 18468.5, 9234.23]"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 0,
					"fontsize" : 12.0,
					"id" : "obj-8",
					"linecount" : 3,
					"maxclass" : "o.compose",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 576.0, 193.0, 649.0, 51.0 ],
					"saved_bundle_data" : [ 35, 98, 117, 110, 100, 108, 101, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -92, 47, 97, 97, 47, 101, 99, 103, 0, 44, 105, 116, 105, 105, 105, 105, 105, 116, 105, 100, 105, 116, 105, 100, 100, 100, 100, 100, 100, 100, 100, 0, 0, 0, 0, -86, 56, -36, -111, 124, 124, -58, 28, 37, -49, 0, 0, 0, -56, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -96, -13, 111, 25, -56, -82, 84, -20, -90, -122, 0, 0, 0, 0, 63, -48, 0, 0, 0, 0, 0, 0, 0, 0, -95, 12, 91, 18, -8, -35, 13, -44, -91, -33, 0, 0, -67, 39, 63, -24, 0, 0, 0, 0, 0, 0, 64, 109, 105, 104, 114, -80, 32, -59, 64, 75, 57, -18, -53, -5, 21, -75, 64, 81, -6, 63, 20, 18, 5, -68, 64, 88, 19, -109, -35, -105, -10, 43, 64, 72, 19, -107, -127, 6, 36, -35, 64, -46, 9, 32, 0, 0, 0, 0, 64, -62, 9, 29, 112, -93, -41, 10 ],
					"saved_bundle_length" : 184,
					"text" : "/aa/ecg : [43576, 2017-04-07T02:48:28.773867Z, 200, 9, 0, 0, 41203, 1959-01-24T14:52:30.331736Z, 0, 0.25, 41228, 1948-06-01T20:06:53.054026Z, 48423, 0.75, 235.294, 54.4526, 71.9101, 96.3059, 48.153, 18468.5, 9234.23]"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-6",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 144.0, 82.0, 126.0, 20.0 ],
					"style" : "",
					"text" : "o.rtecg.timetag.arrival"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-5",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "FullPacket" ],
					"patching_rect" : [ 144.0, 118.0, 69.0, 20.0 ],
					"style" : "",
					"text" : "o.rtecg.self"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-1",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 144.0, 41.0, 131.0, 20.0 ],
					"style" : "",
					"text" : "udpreceive #1 CNMAT"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "obj-4", 0 ],
					"order" : 0,
					"source" : [ "obj-1", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-6", 0 ],
					"order" : 1,
					"source" : [ "obj-1", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-7", 0 ],
					"order" : 2,
					"source" : [ "obj-1", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-6", 0 ],
					"source" : [ "obj-10", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-2", 0 ],
					"source" : [ "obj-11", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-12", 0 ],
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-11", 0 ],
					"source" : [ "obj-5", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-3", 0 ],
					"order" : 0,
					"source" : [ "obj-6", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-5", 0 ],
					"order" : 1,
					"source" : [ "obj-6", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-6", 0 ],
					"source" : [ "obj-8", 0 ]
				}

			}
 ],
		"dependency_cache" : [ 			{
				"name" : "o.rtecg.self.maxpat",
				"bootpath" : "~/Development/maccallum/rtecg/Applications/Max",
				"patcherrelativepath" : ".",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "o.rtecg.timetag.arrival.maxpat",
				"bootpath" : "~/Development/maccallum/rtecg/Applications/Max",
				"patcherrelativepath" : ".",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "o.rtecg.lncache.maxpat",
				"bootpath" : "~/Development/maccallum/rtecg/Applications/Max",
				"patcherrelativepath" : ".",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "o.expr.codebox.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "o.timetag.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "o.compose.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "o.display.mxo",
				"type" : "iLaX"
			}
 ],
		"autosave" : 0
	}

}
