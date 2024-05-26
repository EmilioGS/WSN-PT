//AQUI VA LA CONSULTA A LA BASE DE DATOS
const mysql = require('mysql2');
const fs = require('fs');
const express = require('express');
const app = express();


var config =
{//var conn = mysql.createConnection({host: {your_host}, user: {username@servername}, password: {your_password}, database: {your_database}, Port: {your_port}[, ssl:{ca:fs.readFileSync({ca-cert filename})}}]);
    host:'huertalia.mysql.database.azure.com',
    user:'huertalia',
    password:'Proyecto5$',
    database:'huertalia',
    port: 3306,
    ssl: {rejectUnauthorized: true, ca: fs.readFileSync("./DigiCertGlobalRootCA.crt.pem")},
    connectTimeout: 30000
};

const conn = new mysql.createConnection(config);

/*conn.connect(
    function (err) {
        if (err) {
            console.log("!!! Cannot connect !!! Error:");
            throw err;
        }
        else {
            console.log("Connection established.");
            readData();
        }
    });

function readData(){
    conn.query('SELECT * FROM Usuario',
        function (err, results, fields) {
            if (err) throw err;
            else console.log('Selected ' + results.length + ' row(s).');
            for (i = 0; i < results.length; i++) {
                console.log('Row: ' + JSON.stringify(results[i]));
            }
            console.log('Done.');
        })
    conn.end(
        function (err) {
            if (err) throw err;
            else  console.log('Closing connection.')
    });
};*/

module.exports=conn;