//Import express
const express = require('express');
const app = express();//create an express object called app

//Import RouterLogIn which contains the index controller and the URL /log-in
const routerLogIn = express.Router(); 

const title='Inicio de sesión';

//Render the log-in.pug on the routerLogIn URL
routerLogIn.get('/',(req,res)=>{//metodo y ruta
    res.render('log-in', {title}); //('file.pug,{const to render})
});

routerLogIn.get('/register',(req,res)=>{//metodo y ruta
    res.render('registerUserView'); //('file.pug,{const to render})
});


module.exports= routerLogIn; //export routerLogIn



