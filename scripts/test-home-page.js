'use strict';
// Dependency-free DOM contract harness. This is not a CEF GUI test.
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const html = fs.readFileSync(path.join(__dirname, '../resources/home.html'), 'utf8');
const script = html.match(/<script>([\s\S]*?)<\/script>/)[1];
function harness(saved, storageThrows = false) {
  const handlers = {};
  const input = {value: '', placeholder: ''};
  const engine = {value: 'ddg', options: [{text:'DuckDuckGo'}, {text:'Google'}, {text:'Bing'}], get selectedIndex(){return ['ddg','google','bing'].indexOf(this.value);}, addEventListener(name, fn){handlers['engine:'+name]=fn;}};
  const form = {addEventListener(name, fn){handlers['form:'+name]=fn;}};
  const window = {location:{href:''}};
  const context = vm.createContext({document:{getElementById(id){return {searchInput:input,engine,searchForm:form}[id];}},window,localStorage:{getItem(){if(storageThrows)throw Error('unavailable');return saved;},setItem(){if(storageThrows)throw Error('unavailable');}}});
  vm.runInContext(script, context);
  return {input,engine,window,handlers};
}
let test = harness('invalid');
assert.equal(test.engine.value,'ddg');
test.input.value='youtube.com';
test.handlers['form:submit']({preventDefault(){}});
assert.equal(test.window.location.href,'https://youtube.com');
test = harness('google');
test.input.value='C++ browser';
test.handlers['form:submit']({preventDefault(){}});
assert.equal(test.window.location.href,'https://www.google.com/search?q=C%2B%2B%20browser');
test.engine.value='bing';test.handlers['engine:change']();
assert.match(test.input.placeholder,/Bing/);
test.input.value='https://www.wikipedia.org';test.handlers['form:submit']({preventDefault(){}});
assert.equal(test.window.location.href,'https://www.wikipedia.org');
test.input.value='   ';test.window.location.href='unchanged';test.handlers['form:submit']({preventDefault(){}});
assert.equal(test.window.location.href,'unchanged');
test = harness(null,true);test.handlers['engine:change']();
assert.equal(test.engine.value,'ddg');
assert.equal((html.match(/<a href="https:/g)||[]).length,6);
assert(!html.includes('Zero Telemetry'));
console.log('PASS: home search, engine selection, URL navigation, empty input, unavailable storage and link contracts');
