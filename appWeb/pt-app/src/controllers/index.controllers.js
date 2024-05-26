//Import express
const express = require('express');
const app = express();//create an express object called app

//Import RouterLogIn which contains the index controller and the URL /log-in
const routerLogIn = express.Router(); 

const title='MI FORMULARIO';

//Render the log-in.pug on the routerLogIn URL
routerLogIn.get('/',(req,res)=>{//metodo y ruta
    res.render('log-in-form', {title}); //('file.pug,{const to render})
});

module.exports= routerLogIn; //export routerLogIn



