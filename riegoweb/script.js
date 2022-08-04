const xhr= new XMLHttpRequest();

xhr.addEventListener("load",onRequestHandler);

xhr.open('GET','http://riegoapi.somee.com/api/historials');
xhr.send();
var fecha="";
var dato=0;

function onRequestHandler(){

    if( this.readyState==4 && this.status==200)
    {
        const data=JSON.parse(this.response);
        console.log(data);

        fecha=data[data.length-1].fecha;
        dato=data[data.length-1].dato;
        var tabla="";

        tabla+=" ";
        tabla+="<label style='font-size: xx-large;color:white;'>Fecha y hora:</label><label style='font-size: xx-large; color:lightblue'>  "+ fecha +"</label><br>";
        tabla+="<label style='font-size: xx-large;color:white;'>Humedad:</label><label style='font-size: xx-large; background:black;color:yellow'>  "+ dato +"%</label><br>";

        tabla+="<h2 style='color:white;'>HISTÓRICO DEL SENSOR DE HUMEDAD</h2>";
        /*tabla+="<div style='margin: 0 auto;width: 300px;height: 400px; overflow: auto';>";
        tabla+="<table  style='margin: 0 auto; '><tr style='background:blue;color:yellow'><td>ID</td><td>FECHA</td><td>DATO</td></tr>"
        for (var i = data.length-1; i >= 0; i-=1) {
            let historial=data[i];
            tabla=tabla+"<tr  style='color:gray; '><td>"+historial.id+"</td><td>"+historial.fecha+"</td><td>"+historial.dato+"</td></tr>";
          }
          tabla+="</table>";*/
          tabla+="</div>";

          const HTMLResponse=document.querySelector('#app');
          HTMLResponse.innerHTML=tabla;
    }
}

